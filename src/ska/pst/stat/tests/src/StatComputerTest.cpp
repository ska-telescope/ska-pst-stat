/*
 * Copyright 2023 Square Kilometre Array Observatory
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cmath>
#include <vector>
#include <spdlog/spdlog.h>

#include "ska/pst/common/utils/PacketGeneratorFactory.h"
#include "ska/pst/stat/tests/StatComputerTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"


auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatComputerTest::StatComputerTest()
    : ::testing::Test()
{
}

void StatComputerTest::SetUp()
{
  data_config.load_from_file(test_data_file("stat_computer_config.txt"));
  weights_config = get_weights_config(data_config);
}

void StatComputerTest::TearDown()
{
}

auto StatComputerTest::get_weights_config(const ska::pst::common::AsciiHeader& _data_config) -> ska::pst::common::AsciiHeader
{
  ska::pst::common::AsciiHeader wts_cfg (_data_config);
  wts_cfg.set("NDIM",1);
  wts_cfg.set("NPOL",1);
  return wts_cfg;
}

void StatComputerTest::configure(bool use_generator)
{
  storage = std::make_shared<StatStorage>(data_config);
  // this should come from config
  uint32_t nfreq_bins{0};
  if (data_config.has("STAT_REQ_FREQ_BINS")) {
    nfreq_bins = data_config.get_uint32("STAT_REQ_FREQ_BINS");
  } else {
    throw std::runtime_error("STAT_REQ_FREQ_BINS not set in test configuration");
  }

  uint32_t ntime_bins{0};
  if (data_config.has("STAT_REQ_TIME_BINS")) {
    ntime_bins = data_config.get_uint32("STAT_REQ_TIME_BINS");
  } else {
    throw std::runtime_error("STAT_REQ_TIME_BINS not set in test configuration");
  }

  storage->resize(ntime_bins, nfreq_bins);

  layout = std::make_shared<TestDataLayout>(data_config);
  if (use_generator) {
    generator = ska::pst::common::PacketGeneratorFactory("GaussianNoise", layout);
    generator->configure(data_config);
  }
  computer = std::make_unique<ska::pst::stat::StatComputer>(data_config, weights_config, storage);
  computer->initialise();

  ndim = data_config.get_uint32("NDIM");
  npol = data_config.get_uint32("NPOL");
  nbit = data_config.get_uint32("NBIT");
  nchan = data_config.get_uint32("NCHAN");

  auto nsamp_per_packet = data_config.get_uint32("UDP_NSAMP");
  auto nchan_per_packet = data_config.get_uint32("UDP_NCHAN");

  data_packet_stride = layout->get_packet_data_size();

  SPDLOG_DEBUG("layout->get_packet_scales_size()={}", layout->get_packet_scales_size()); //NOLINT
  SPDLOG_DEBUG("layout->get_packet_weights_size()={}", layout->get_packet_weights_size()); //NOLINT

  weights_packet_stride = layout->get_packet_weights_size() + layout->get_packet_scales_size();
  packet_resolution = nsamp_per_packet * nchan_per_packet * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  heap_resolution = nsamp_per_packet * nchan * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  packets_per_heap = heap_resolution / packet_resolution;

  SPDLOG_DEBUG("data_packet_stride={}", data_packet_stride); //NOLINT
  SPDLOG_DEBUG("weights_packet_stride={}", weights_packet_stride); //NOLINT
  SPDLOG_DEBUG("packet_resolution={}", packet_resolution); //NOLINT
  SPDLOG_DEBUG("heap_resolution={}", heap_resolution); //NOLINT
  SPDLOG_DEBUG("packets_per_heap={}", packets_per_heap); //NOLINT
}

void StatComputerTest::generate_packets(const uint32_t num_packets)
{
  SPDLOG_DEBUG("StatComputerTest::generate_packets()");
  if (generator == nullptr) {
    throw std::runtime_error("Test requested to generate packets but configured not to use a generator");
  }

  SPDLOG_DEBUG("StatComputerTest::generate_packets generating {} packets", num_packets);
  // we need to resize and reset data and weights
  auto total_weights_size = num_packets * weights_packet_stride;
  auto total_data_size = num_packets * layout->get_packet_data_size();
  SPDLOG_DEBUG("StatComputerTest::generate_packets resizing weights buffer to size {}", total_weights_size);
  weights_buffer.resize(total_weights_size);
  SPDLOG_DEBUG("StatComputerTest::generate_packets resizing data buffer to size {}", total_data_size);
  data_buffer.resize(total_data_size);

  for (auto packet_number = 0; packet_number < num_packets; packet_number++) {
    auto data_offset = packet_number * layout->get_packet_data_size();
    auto weights_offset = packet_number * weights_packet_stride;

    SPDLOG_TRACE("StatComputerTest::generate_packets generating data packet {}", packet_number);
    generator->fill_data(data_buffer.data() + data_offset, layout->get_packet_data_size()); // NOLINT
    SPDLOG_TRACE("StatComputerTest::generate_packets generating scales packet {}", packet_number);
    generator->fill_scales(weights_buffer.data() + weights_offset + layout->get_packet_scales_offset(), layout->get_packet_scales_size()); // NOLINT
    SPDLOG_TRACE("StatComputerTest::generate_packets generating weights packet {}", packet_number);
    generator->fill_weights(weights_buffer.data() + weights_offset + layout->get_packet_weights_offset(), layout->get_packet_weights_size()); // NOLINT
  }
}

TEST_F(StatComputerTest, test_compute) // NOLINT
{
  SPDLOG_DEBUG("StatComputerTest::test_compute - start");
  configure(true);

  SPDLOG_DEBUG("StatComputerTest::test_compute - generating packets");
  uint32_t nheaps = 1;
  uint32_t num_packets = nheaps * packets_per_heap;

  generate_packets(num_packets);
  SPDLOG_DEBUG("StatComputerTest::test_compute - finished generating packets");

  SPDLOG_DEBUG("StatComputerTest::test_compute - computing first packet");

  ska::pst::common::SegmentProducer::Segment segment;
  segment.data.block = data_buffer.data();
  segment.data.size = num_packets * layout->get_packet_data_size();
  segment.weights.block = weights_buffer.data();
  segment.weights.size = num_packets * weights_packet_stride;
  computer->compute(segment);

  for (auto ipol = 0; ipol < storage->get_npol(); ipol++) {
    for (auto idim = 0; idim < storage->get_ndim(); idim++) {
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_frequency_avg[{}][{}]={}", ipol, idim, storage->mean_frequency_avg[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_frequency_avg[{}][{}]={}", ipol, idim, storage->variance_frequency_avg[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_frequency_avg_masked[{}][{}]={}", ipol, idim, storage->mean_frequency_avg_masked[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_frequency_avg_masked[{}][{}]={}", ipol, idim, storage->variance_frequency_avg_masked[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->num_clipped_samples[{}][{}]={}", ipol, idim, storage->num_clipped_samples[ipol][idim]);
      for (auto ichan = 0; ichan < storage->get_nchan(); ichan++) {
        SPDLOG_TRACE("StatComputerTest::test_compute - storage->mean_spectrum[{}][{}][{}]={}", ipol, idim, ichan, storage->mean_spectrum[ipol][idim][ichan]);
        SPDLOG_TRACE("StatComputerTest::test_compute - storage->variance_spectrum[{}][{}][{}]={}", ipol, idim, ichan, storage->variance_spectrum[ipol][idim][ichan]);
      }
    }
    for (auto ichan = 0; ichan < storage->get_nchan(); ichan++) {
      SPDLOG_TRACE("StatComputerTest::test_compute - storage->mean_spectral_power[{}][{}]={}", ipol, ichan, storage->mean_spectral_power[ipol][ichan]);
      SPDLOG_TRACE("StatComputerTest::test_compute - storage->max_spectral_power[{}][{}]={}", ipol, ichan, storage->max_spectral_power[ipol][ichan]);
    }
  }

  // assert frequency_bins and timeseries_bins
  auto nfreq_bins = storage->get_nfreq_bins();
  auto centre_freq = data_config.get_double("FREQ");
  auto bandwidth = data_config.get_double("BW");
  auto bandwidth_per_bin = bandwidth / static_cast<double>(nfreq_bins);
  auto start_freq = centre_freq - bandwidth / 2.0; // NOLINT
  SPDLOG_DEBUG("centre_freq={}, bandwidth={}, nfreq_bins={}, bandwidth_per_bin={}, start_freq={}", centre_freq, bandwidth, nfreq_bins, bandwidth_per_bin, start_freq);

  for (auto freq_bin = 0; freq_bin < nfreq_bins; freq_bin++)
  {
    double expected_freq_bin = start_freq + bandwidth_per_bin * static_cast<double>(2 * freq_bin + 1) / 2.0; // NOLINT
    EXPECT_DOUBLE_EQ(storage->frequency_bins[freq_bin], expected_freq_bin);
  }

  auto ntime_bins = storage->get_ntime_bins();
  auto total_sample_time = storage->get_total_sample_time();
  auto sample_time_per_bin = total_sample_time / ntime_bins;
  auto tsamp = data_config.get_double("TSAMP");
  auto nsamp_per_packet = data_config.get_double("UDP_NSAMP");
  EXPECT_DOUBLE_EQ(total_sample_time, tsamp * static_cast<double>(nsamp_per_packet * nheaps) * static_cast<double>(ska::pst::common::seconds_per_microseconds));

  double expected_time = sample_time_per_bin / 2.0; // NOLINT
  for (auto time_bin = 0; time_bin < ntime_bins; time_bin++)
  {
    EXPECT_DOUBLE_EQ(storage->timeseries_bins[time_bin], expected_time);
    expected_time += sample_time_per_bin;
  }
}

TEST_F(StatComputerTest, test_expected_values) // NOLINT
{
  data_config.reset();
  data_config.load_from_file(test_data_file("stat_computer_1chan_32nsamp_config.txt"));
  weights_config = get_weights_config(data_config);
  configure(false);

  // This is gausian data with mean of 3.14, stddev of 10, rounded to nearest int
  // There is only 1 channel, 2 pol, 2 dims, 32 samples each (128 values)
  std::vector<int16_t> data = {
    // Pol A - sample 1 - 8
     -4,  19,  17,   6,  -2,   2,   0,  15,  15,   3,  15,   8, -11, -21, -18,   2, // NOLINT
    // Pol A - sample 9 - 16
    -11,   9,  -3,   5,  -4, -13,  12,  -1,   5,  10,  21,   0,  25,  -2,   0,  12, // NOLINT
    // Pol A - sample 17 - 24
      8,  -6,  -8,  23, -11,  -6,  28,   3,  32,  -2,  17,   6,  -8,   4,  -9,   0, // NOLINT
    // Pol A - sample 25 - 23
     12,   6,  -9, -18,  -5,   0, -12,   1,  12,   9, -18,   8,   9,   2,   0,  -8, // NOLINT
    // Pol B - sample 1 - 8
      4,   9,   0,  14,  24,   0,  17,   2,  -5,   0,   7,  11,   8,  -3,   2,  12, // NOLINT
    // Pol B - sample 9 - 16
      8,   8,  19,   3,  13,  22,  -2, -10, -13,  19,  -1,  16,  -2,   2,   0,  -3, // NOLINT
    // Pol B - sample 17 - 24
      1, -23,  -1,  32,   1,  15,   5,  10,  -1,  20,  -1,   6,  15, -13,  -4,   5, // NOLINT
    // Pol B - sample 25 - 23
     -1,   5,  -1,   1,  12,  -3,  -6,  -6,   0,  -5,  15,  12,  20,  13,  -2,  21  // NOLINT
  };

  auto data_length = data.size() * sizeof(int16_t);
  auto data_block = reinterpret_cast<char*>(data.data());

  unsigned weights_length = sizeof(float) + sizeof(uint16_t);
  std::vector<char> weights(weights_length);
  auto scale = reinterpret_cast<float*>(weights.data());
  *scale = 1.0;
  auto wt = reinterpret_cast<uint16_t*>(weights.data() + sizeof(float));
  *wt = 1;

  ska::pst::common::SegmentProducer::Segment segment;
  segment.data.block = data_block;
  segment.data.size = data_length;
  segment.weights.block = weights.data();
  segment.weights.size = weights_length;
  computer->compute(segment);

  // [0,1][0,1] are [pol][dim]
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[0][0], 2.96875);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[0][0], 185.0635081);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[0][0], 2.96875);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[0][0], 185.0635081);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[0][1], 2.375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[0][1], 87.98387097);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[0][1], 2.375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[0][1], 87.98387097);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[1][0], 4.09375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[1][0], 75.50705645);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[1][0], 4.09375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[1][0], 75.50705645);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[1][1], 6);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[1][1], 130.5806452);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[1][1], 6);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[1][1], 130.5806452);

  // assert no clipping
  for (auto ipol = 0; ipol < npol; ipol++)
  {
    for (auto idim = 0; idim < ndim; idim++)
    {
      ASSERT_EQ(storage->num_clipped_samples[ipol][idim], 0);
      for (auto ichan = 0; ichan < nchan; ichan++)
      {
        ASSERT_EQ(storage->num_clipped_samples_spectrum[ipol][idim][ichan], 0);
      }
    }
  }

  // [0,1][0,1][x] are [pol][dim][chan]
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][0][0], 2.96875);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][0][0], 185.0635081);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][1][0], 2.375);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][1][0], 87.98387097);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][0][0], 4.09375);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][0][0], 75.50705645);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][1][0], 6);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][1][0], 130.5806452);

  // [0,1][0,1][x] are [pol][dim]
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[0][0], 278.96875);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[0][0], 1028);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[1][0], 252.40625);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[1][0], 1025);

  for (auto ipol = 0; ipol < storage->get_npol(); ipol++) {
    for (auto idim = 0; idim < storage->get_ndim(); idim++) {
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_frequency_avg[{}][{}]={}",
        ipol, idim, storage->mean_frequency_avg[ipol][idim]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_frequency_avg[{}][{}]={}",
        ipol, idim, storage->variance_frequency_avg[ipol][idim]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_frequency_avg_masked[{}][{}]={}",
        ipol, idim, storage->mean_frequency_avg_masked[ipol][idim]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_frequency_avg_masked[{}][{}]={}",
        ipol, idim, storage->variance_frequency_avg_masked[ipol][idim]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->num_clipped_samples[{}][{}]={}",
        ipol, idim, storage->num_clipped_samples[ipol][idim]
      );
      for (auto ichan = 0; ichan < storage->get_nchan(); ichan++) {
        SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_spectrum[{}][{}][{}]={}",
          ipol, idim, ichan, storage->mean_spectrum[ipol][idim][ichan]
        );
        SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_spectrum[{}][{}][{}]={}",
          ipol, idim, ichan, storage->variance_spectrum[ipol][idim][ichan]
        );
      }
    }
    for (auto ichan = 0; ichan < storage->get_nchan(); ichan++) {
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_spectral_power[{}][{}]={}",
        ipol, ichan, storage->mean_spectral_power[ipol][ichan]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->max_spectral_power[{}][{}]={}",
        ipol, ichan, storage->max_spectral_power[ipol][ichan]
      );
    }
  }
}

TEST_F(StatComputerTest, test_masked_channels) // NOLINT
{
  data_config.reset();
  data_config.load_from_file(test_data_file("stat_computer_4chan_8nsamp_masked_config.txt"));
  weights_config = get_weights_config(data_config);
  configure(false);

  // This is gausian data with mean of 3.14, stddev of 10, rounded to nearest int
  // There are 4 channels, 2 pols, 2 dims, and 8 samples each (128 values)
  std::vector<int16_t> data = {
    // Pol A - channel 1
     -4,  19,  17,   6,  -2,   2,   0,  15,  15,   3,  15,   8, -11, -21, -18,   2, // NOLINT
    // Pol A - channel 2
    -11,   9,  -3,   5,  -4, -13,  12,  -1,   5,  10,  21,   0,  25,  -2,   0,  12, // NOLINT
    // Pol A - channel 3
      8,  -6,  -8,  23, -11,  -6,  28,   3,  32,  -2,  17,   6,  -8,   4,  -9,   0, // NOLINT
    // Pol A - channel 4
     12,   6,  -9, -18,  -5,   0, -12,   1,  12,   9, -18,   8,   9,   2,   0,  -8, // NOLINT
    // Pol B - channel 1
      4,   9,   0,  14,  24,   0,  17,   2,  -5,   0,   7,  11,   8,  -3,   2,  12, // NOLINT
    // Pol B - channel 2
      8,   8,  19,   3,  13,  22,  -2, -10, -13,  19,  -1,  16,  -2,   2,   0,  -3, // NOLINT
    // Pol B - channel 3
      1, -23,  -1,  32,   1,  15,   5,  10,  -1,  20,  -1,   6,  15, -13,  -4,   5, // NOLINT
    // Pol B - channel 4
     -1,   5,  -1,   1,  12,  -3,  -6,  -6,   0,  -5,  15,  12,  20,  13,  -2,  21  // NOLINT
  };

  auto data_length = data.size() * sizeof(int16_t);
  auto data_block = reinterpret_cast<char*>(data.data());

  unsigned weights_length = sizeof(float) + nchan * sizeof(uint16_t);
  std::vector<char> weights(weights_length);
  auto scale = reinterpret_cast<float*>(weights.data());
  auto wt = reinterpret_cast<uint16_t*>(weights.data() + sizeof(float));

  *scale = 1.0;
  for (unsigned i=0; i<nchan; i++)
  {
    wt[i] = 1;
  }

  ska::pst::common::SegmentProducer::Segment segment;
  segment.data.block = data_block;
  segment.data.size = data_length;
  segment.weights.block = weights.data();
  segment.weights.size = weights_length;
  computer->compute(segment);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[0][0], 2.96875);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[0][0], 185.0635081);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[0][0], 2.375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[0][0], 222.9166667);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[0][1], 2.375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[0][1], 87.98387097);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[0][1], 1.375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[0][1], 80.65);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[1][0], 4.09375);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[1][0], 75.50705645);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[1][0], 3.25);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[1][0], 60.86666667);

  ASSERT_FLOAT_EQ(storage->mean_frequency_avg[1][1], 6);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg[1][1], 130.5806452);
  ASSERT_FLOAT_EQ(storage->mean_frequency_avg_masked[1][1], 5.625);
  ASSERT_FLOAT_EQ(storage->variance_frequency_avg_masked[1][1], 190.1166667);

  ASSERT_EQ(storage->num_clipped_samples[0][1], 0);
  ASSERT_EQ(storage->num_clipped_samples[1][0], 0);
  ASSERT_EQ(storage->num_clipped_samples[1][1], 0);

  // assert no clipping
  for (auto ipol = 0; ipol < npol; ipol++)
  {
    for (auto idim = 0; idim < ndim; idim++)
    {
      ASSERT_EQ(storage->num_clipped_samples[ipol][idim], 0);
      for (auto ichan = 0; ichan < nchan; ichan++)
      {
        ASSERT_EQ(storage->num_clipped_samples_spectrum[ipol][idim][ichan], 0);
      }
    }
  }

  // channel 1
  // [0,1][0,1][0] are [pol][dim][chan]
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][0][0], 1.5);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][0][0], 169.4285714);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][1][0], 4.25);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][1][0], 142.7857143);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][0][0], 7.125);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][0][0], 88.125);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][1][0], 5.625);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][1][0], 43.125);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[0][0], 293.5);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[0][0], 562);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[1][0], 197.25);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[1][0], 576);

  // channel 2
  // [0,1][0,1][1] are [pol][dim][chan]
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][0][1], 5.625);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][0][1], 161.125);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][1][1], 2.5);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][1][1], 67.71428571);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][0][1], 2.75);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][0][1], 101.6428571);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][1][1], 7.125);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][1][1], 125.8392857);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[0][1], 238.125);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[0][1], 629);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[1][1], 257.375);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[1][1], 653);

  // channel 3
  // [0,1][0,1][2] are [pol][dim][chan]
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][0][2], 6.125);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][0][2], 312.9821429);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][1][2], 2.75);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][1][2], 86.5);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][0][2], 1.875);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][0][2], 34.69642857);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][1][2], 6.5);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][1][2], 310);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[0][2], 394.625);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[0][2], 1028);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[1][2], 347.375);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[1][2], 1025);

  // channel 4
  // [0,1][0,1][3] are [pol][dim][chan]
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][0][3], -1.375);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][0][3], 132.5535714);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][1][3], 0);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][1][3], 82);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][0][3], 4.625);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][0][3], 91.41071429);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][1][3], 4.75);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][1][3], 95.64285714);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[0][3], 189.625);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[0][3], 405);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[1][3], 207.625);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[1][3], 569);

  for (auto ipol = 0; ipol < storage->get_npol(); ipol++)
  {
    for (auto freq_bin = 0; freq_bin < storage->get_nfreq_bins(); freq_bin++)
    {
      for (auto time_bin = 0; time_bin < storage->get_ntime_bins(); time_bin++)
      {
        SPDLOG_DEBUG("StatComputerTest::test_compute - storage->spectrogram[{}][{}][{}]={}",
          ipol, freq_bin, time_bin, storage->spectrogram[ipol][freq_bin][time_bin]
        );
      }
    }
    for (auto time_bin = 0; time_bin < storage->get_ntime_bins(); time_bin++)
    {
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->timeseries[{}][{}][MAX]={}",
        ipol, time_bin, storage->timeseries[ipol][time_bin][0]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->timeseries[{}][{}][MIN]={}",
        ipol, time_bin, storage->timeseries[ipol][time_bin][1]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->timeseries[{}][{}][MEAN]={}",
        ipol, time_bin, storage->timeseries[ipol][time_bin][2]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->timeseries_masked[{}][{}][MAX]={}",
        ipol, time_bin, storage->timeseries_masked[ipol][time_bin][0]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->timeseries_masked[{}][{}][MIN]={}",
        ipol, time_bin, storage->timeseries_masked[ipol][time_bin][1]
      );
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->timeseries_masked[{}][{}][MEAN]={}",
        ipol, time_bin, storage->timeseries_masked[ipol][time_bin][2]
      );
    }
  }

  // assertions of spectrogram
  // [0,1][0,1][0] are [pol][freq_bin][temp_bin]
  ASSERT_FLOAT_EQ(storage->spectrogram[0][0][0], 938);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][0][1], 563);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][0][2], 1089);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][0][3], 1663);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][1][0], 1278);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][1][1], 1120);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][1][2], 1966);
  ASSERT_FLOAT_EQ(storage->spectrogram[0][1][3], 310);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][0][0], 791);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][0][1], 1626);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][0][2], 982);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][0][3], 238);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][1][0], 1583);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][1][1], 576);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][1][2], 832);
  ASSERT_FLOAT_EQ(storage->spectrogram[1][1][3], 1449);

  // for the assertions below of timeseries/timeseris_masked
  // the [0,1][0,1,2,3][0,1,2] = [pol][temp_bin][max,min,mean]

  // assertions of timeseries (Pol A - max)
  ASSERT_FLOAT_EQ(storage->timeseries[0][0][0], 593);
  ASSERT_FLOAT_EQ(storage->timeseries[0][1][0], 793);
  ASSERT_FLOAT_EQ(storage->timeseries[0][2][0], 1028);
  ASSERT_FLOAT_EQ(storage->timeseries[0][3][0], 629);

  // assertions of timeseries (Pol A - min)
  ASSERT_FLOAT_EQ(storage->timeseries[0][0][1], 34);
  ASSERT_FLOAT_EQ(storage->timeseries[0][1][1], 8);
  ASSERT_FLOAT_EQ(storage->timeseries[0][2][1], 125);
  ASSERT_FLOAT_EQ(storage->timeseries[0][3][1], 64);

  // assertions of timeseries (Pol A - mean)
  ASSERT_FLOAT_EQ(storage->timeseries[0][0][2], 277);
  ASSERT_FLOAT_EQ(storage->timeseries[0][1][2], 210.375);
  ASSERT_FLOAT_EQ(storage->timeseries[0][2][2], 381.875);
  ASSERT_FLOAT_EQ(storage->timeseries[0][3][2], 246.625);

  // assertions of timeseries (Pol B - max)
  ASSERT_FLOAT_EQ(storage->timeseries[1][0][0], 1025);
  ASSERT_FLOAT_EQ(storage->timeseries[1][1][0], 653);
  ASSERT_FLOAT_EQ(storage->timeseries[1][2][0], 530);
  ASSERT_FLOAT_EQ(storage->timeseries[1][3][0], 569);

  // assertions of timeseries (Pol B - min)
  ASSERT_FLOAT_EQ(storage->timeseries[1][0][1], 2);
  ASSERT_FLOAT_EQ(storage->timeseries[1][1][1], 72);
  ASSERT_FLOAT_EQ(storage->timeseries[1][2][1], 25);
  ASSERT_FLOAT_EQ(storage->timeseries[1][3][1], 8);

  // assertions of timeseries (Pol B - mean)
  ASSERT_FLOAT_EQ(storage->timeseries[1][0][2], 296.75);
  ASSERT_FLOAT_EQ(storage->timeseries[1][1][2], 275.25);
  ASSERT_FLOAT_EQ(storage->timeseries[1][2][2], 226.75);
  ASSERT_FLOAT_EQ(storage->timeseries[1][3][2], 210.875);

  // assertions of timeseries_masked (Pol A - max)
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][0][0], 593);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][1][0], 793);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][2][0], 1028);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][3][0], 85);

  // assertions of timeseries_masked (Pol A - min)
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][0][1], 100);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][1][1], 25);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][2][1], 225);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][3][1], 64);

  // assertions of timeseries_masked (Pol A - mean)
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][0][2], 319.5);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][1][2], 280);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][2][2], 491.5);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[0][3][2], 77.5);

  // assertions of timeseries_masked (Pol B - max)
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][0][0], 1025);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][1][0], 226);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][2][0], 401);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][3][0], 569);

  // assertions of timeseries_masked (Pol B - min)
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][0][1], 2);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][1][1], 72);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][2][1], 25);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][3][1], 41);

  // assertions of timeseries_masked (Pol B - mean)
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][0][2], 395.75);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][1][2], 144);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][2][2], 208);
  ASSERT_FLOAT_EQ(storage->timeseries_masked[1][3][2], 362.25);

}

TEST_F(StatComputerTest, test_clipped_channels) // NOLINT
{
  data_config.reset();
  data_config.load_from_file(test_data_file("stat_computer_4chan_8nsamp_config.txt"));
  weights_config = get_weights_config(data_config);
  configure(false);

  // This is gausian data with mean of 3.14, stddev of 10, rounded to nearest int
  // There are 4 channels, 2 pols, 2 dims, and 8 samples each (128 values).
  // Some values have been put in a bin that should be masked
  std::vector<int16_t> data = {
    // Pol A - channel 1
    -32768,  19,  17,   6,  -2,   2,   0,  15,  15,   3,  15,   8, -11, -21, -18,   2,   // NOLINT
    // Pol A - channel 2
    -11,   32767,  -3,   5,  -4, -13,  12,  -1,   5,  10,  21,   0,  25,  -2,   0,  12,  // NOLINT
    // Pol A - channel 3
      8,  -6,  -32768,  23, -11,  -6,  28,   3,  32,  -2,  17,   6,  -8,   4,  -9,   0,  // NOLINT
    // Pol A - channel 4
     12,   6,  -9, 32767,  -5,   0, -12,   1,  12,   9, -18,   8,   9,   2,   0,  -8,    // NOLINT
    // Pol B - channel 1
      4,   9,   0,  14,  32767,   0,  17,   2,  -5,   0,   7,  11,   8,  -3,   2,  12,   // NOLINT
    // Pol B - channel 2
      8,   8,  19,   3,  13,  32767,  -2, -10, -13,  19,  -1,  16,  -2,   2,   0,  -3,   // NOLINT
    // Pol B - channel 3
      1, -23,  -1,  32,   1,  15,   32767,  10,  -1,  20,  -1,   6,  15, -13,  -4,   5,  // NOLINT
    // Pol B - channel 4
     -1,   5,  -1,   1,  12,  -3,  -6,  -32768,   0,  -5,  15,  12,  20,  13,  -2,  21   // NOLINT
  };

  auto data_length = data.size() * sizeof(int16_t);
  auto data_block = reinterpret_cast<char*>(data.data());

  unsigned weights_length = sizeof(float) + nchan * sizeof(uint16_t);
  std::vector<char> weights(weights_length);
  auto scale = reinterpret_cast<float*>(weights.data());
  auto wt = reinterpret_cast<uint16_t*>(weights.data() + sizeof(float));

  *scale = 1.0;
  for (unsigned i=0; i<nchan; i++)
  {
    wt[i] = 1;
  }

  ska::pst::common::SegmentProducer::Segment segment;
  segment.data.block = data_block;
  segment.data.size = data_length;
  segment.weights.block = weights.data();
  segment.weights.size = weights_length;
  computer->compute(segment);

  // [0,1][0,1] are [pol][dim]
  ASSERT_EQ(storage->num_clipped_samples[0][0], 2);
  ASSERT_EQ(storage->num_clipped_samples[0][1], 2);
  ASSERT_EQ(storage->num_clipped_samples[1][0], 2);
  ASSERT_EQ(storage->num_clipped_samples[1][1], 2);

  // [0,1][0,1][x] are [pol][dim][chan]
  // channel 1
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][0][0], 1);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][1][0], 0);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][0][0], 1);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][1][0], 0);

  // channel 2
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][0][1], 0);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][1][1], 1);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][0][1], 0);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][1][1], 1);

  // channel 3
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][0][2], 1);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][1][2], 0);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][0][2], 1);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][1][2], 0);

  // channel 4
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][0][3], 0);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[0][1][3], 1);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][0][3], 0);
  ASSERT_EQ(storage->num_clipped_samples_spectrum[1][1][3], 1);
}

TEST_F(StatComputerTest, test_nchan_cannot_be_zero) // NOLINT
{
  ska::pst::common::AsciiHeader data_config;
  data_config.load_from_file(test_data_file("stat_computer_1chan_32nsamp_config.txt"));
  data_config.set("NCHAN", 0);
  ska::pst::common::AsciiHeader weights_config = get_weights_config(data_config);

  auto storage = std::make_shared<StatStorage>(data_config);
  EXPECT_ANY_THROW(
    StatComputer(data_config, weights_config, storage);
  );
}

TEST_F(StatComputerTest, test_nbit_cannot_be_zero) // NOLINT
{
  ska::pst::common::AsciiHeader data_config;
  data_config.load_from_file(test_data_file("stat_computer_1chan_32nsamp_config.txt"));
  data_config.set("NBIT", 0);
  ska::pst::common::AsciiHeader weights_config = get_weights_config(data_config);

  auto storage = std::make_shared<StatStorage>(data_config);
  EXPECT_ANY_THROW(
    StatComputer(data_config, weights_config, storage);
  );
}

TEST_F(StatComputerTest, test_npol_cannot_be_zero) // NOLINT
{
  ska::pst::common::AsciiHeader data_config;
  data_config.load_from_file(test_data_file("stat_computer_1chan_32nsamp_config.txt"));
  data_config.set("NPOL", 0);
  ska::pst::common::AsciiHeader weights_config = get_weights_config(data_config);

  auto storage = std::make_shared<StatStorage>(data_config);
  EXPECT_ANY_THROW(
    StatComputer(data_config, weights_config, storage);
  );
}

} // namespace ska::pst::stat::test
