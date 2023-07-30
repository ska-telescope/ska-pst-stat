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

#include "ska/pst/common/utils/DataGeneratorFactory.h"
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
  ska::pst::common::AsciiHeader config;
  config.load_from_file(test_data_file("stat_computer_config.txt"));
  configure(config);
}

void StatComputerTest::TearDown()
{
}

void StatComputerTest::configure(const ska::pst::common::AsciiHeader& config, bool use_generator)
{
  storage = std::make_shared<StatStorage>(config);
  // this should come from config
  storage->resize(1, 1);
  layout = std::make_shared<TestDataLayout>(config);
  if (use_generator) {
    generator = ska::pst::common::DataGeneratorFactory("GaussianNoise", layout);
    generator->configure(config);
  }
  computer = std::make_unique<ska::pst::stat::StatComputer>(config, storage);

  ndim = config.get_uint32("NDIM");
  npol = config.get_uint32("NPOL");
  nbit = config.get_uint32("NBIT");
  nchan = config.get_uint32("NCHAN");

  auto nsamp_per_packet = config.get_uint32("UDP_NSAMP");
  auto nchan_per_packet = config.get_uint32("UDP_NCHAN");
  auto nsamp_per_weight = config.get_uint32("WT_NSAMP");

  data_packet_stride = layout->get_packet_data_size();

  SPDLOG_DEBUG("layout->get_packet_scales_size()={}", layout->get_packet_scales_size()); //NOLINT
  SPDLOG_DEBUG("layout->get_packet_weights_size()={}", layout->get_packet_weights_size()); //NOLINT

  weights_packet_stride = config.get_uint32("PACKET_WEIGHTS_SIZE") + config.get_uint32("PACKET_SCALES_SIZE");
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
    generator->fill_data(data_buffer.data() + data_offset, layout->get_packet_data_size());
    SPDLOG_TRACE("StatComputerTest::generate_packets generating scales packet {}", packet_number);
    generator->fill_scales(weights_buffer.data() + weights_offset + layout->get_packet_scales_offset(), layout->get_packet_scales_size());
    SPDLOG_TRACE("StatComputerTest::generate_packets generating weights packet {}", packet_number);
    generator->fill_weights(weights_buffer.data() + weights_offset + layout->get_packet_weights_offset(), layout->get_packet_weights_size());
  }
}

TEST_F(StatComputerTest, test_compute) // NOLINT
{
  SPDLOG_DEBUG("StatComputerTest::test_compute - start");
  SPDLOG_DEBUG("StatComputerTest::test_compute - generating packets");
  uint32_t num_heaps = 1;
  uint32_t num_packets = num_heaps * packets_per_heap;

  generate_packets(num_packets);
  SPDLOG_DEBUG("StatComputerTest::test_compute - finished generating packets");

  SPDLOG_DEBUG("StatComputerTest::test_compute - computing first packet");
  computer->compute(data_buffer.data(), num_packets * layout->get_packet_data_size(), weights_buffer.data(), num_packets * weights_packet_stride);

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
}

TEST_F(StatComputerTest, test_expected_values) // NOLINT
{
  // This is gausian data with mean of 3.14, stddev of 10, rounded to nearest int
  // There is only 1 channel, 2 pol, 2 dims, 32 samples each (128 values)
  std::vector<int16_t> data = {
    // Pol A - sample 1 - 8
     -4,  19,  17,   6,  -2,   2,   0,  15,  15,   3,  15,   8, -11, -21, -18,   2,
    // Pol A - sample 1 - 8
    -11,   9,  -3,   5,  -4, -13,  12,  -1,   5,  10,  21,   0,  25,  -2,   0,  12,
    // Pol A - sample 17 - 24
      8,  -6,  -8,  23, -11,  -6,  28,   3,  32,  -2,  17,   6,  -8,   4,  -9,   0,
    // Pol A - sample 25 - 23
     12,   6,  -9, -18,  -5,   0, -12,   1,  12,   9, -18,   8,   9,   2,   0,  -8,
    // Pol B - sample 1 - 8
      4,   9,   0,  14,  24,   0,  17,   2,  -5,   0,   7,  11,   8,  -3,   2,  12,
    // Pol B - sample 9 - 16
      8,   8,  19,   3,  13,  22,  -2, -10, -13,  19,  -1,  16,  -2,   2,   0,  -3,
    // Pol B - sample 17 - 24
      1, -23,  -1,  32,   1,  15,   5,  10,  -1,  20,  -1,   6,  15, -13,  -4,   5,
    // Pol B - sample 25 - 23
     -1,   5,  -1,   1,  12,  -3,  -6,  -6,   0,  -5,  15,  12,  20,  13,  -2,  21
  };
  char * data_block = reinterpret_cast<char *>(data.data());
  float scale{1.0};

  ska::pst::common::AsciiHeader config;
  config.set("NDIM", 2);
  config.set("NPOL", 2);
  config.set("NBIT", 16);
  config.set("NCHAN", 1);
  config.set("NMASK", 0);
  config.set("FREQ", 1000);
  config.set("BW", 400);
  config.set("UDP_NSAMP", 32);
  config.set("UDP_NCHAN", 1);
  config.set("WT_NSAMP", 32);
  config.set("PACKET_WEIGHTS_SIZE", 0);
  config.set("PACKET_SCALES_SIZE", 4);
  config.set("STAT_NREBIN", 1);

  configure(config, false);

  computer->compute(data_block, 256, reinterpret_cast<char *>(&scale), 4);

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

  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][0][0], 2.96875);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][0][0], 185.0635081);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[0][1][0], 2.375);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[0][1][0], 87.98387097);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][0][0], 4.09375);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][0][0], 75.50705645);
  ASSERT_FLOAT_EQ(storage->mean_spectrum[1][1][0], 6);
  ASSERT_FLOAT_EQ(storage->variance_spectrum[1][1][0], 130.5806452);

  ASSERT_FLOAT_EQ(storage->mean_spectral_power[0][0], 278.96875);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[0][0], 1028);
  ASSERT_FLOAT_EQ(storage->mean_spectral_power[1][0], 252.40625);
  ASSERT_FLOAT_EQ(storage->max_spectral_power[1][0], 1025);

  for (auto ipol = 0; ipol < storage->get_npol(); ipol++) {
    for (auto idim = 0; idim < storage->get_ndim(); idim++) {
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_frequency_avg[{}][{}]={}", ipol, idim, storage->mean_frequency_avg[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_frequency_avg[{}][{}]={}", ipol, idim, storage->variance_frequency_avg[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_frequency_avg_masked[{}][{}]={}", ipol, idim, storage->mean_frequency_avg_masked[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_frequency_avg_masked[{}][{}]={}", ipol, idim, storage->variance_frequency_avg_masked[ipol][idim]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->num_clipped_samples[{}][{}]={}", ipol, idim, storage->num_clipped_samples[ipol][idim]);
      for (auto ichan = 0; ichan < storage->get_nchan(); ichan++) {
        SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_spectrum[{}][{}][{}]={}", ipol, idim, ichan, storage->mean_spectrum[ipol][idim][ichan]);
        SPDLOG_DEBUG("StatComputerTest::test_compute - storage->variance_spectrum[{}][{}][{}]={}", ipol, idim, ichan, storage->variance_spectrum[ipol][idim][ichan]);
      }
    }
    for (auto ichan = 0; ichan < storage->get_nchan(); ichan++) {
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->mean_spectral_power[{}][{}]={}", ipol, ichan, storage->mean_spectral_power[ipol][ichan]);
      SPDLOG_DEBUG("StatComputerTest::test_compute - storage->max_spectral_power[{}][{}]={}", ipol, ichan, storage->max_spectral_power[ipol][ichan]);
    }
  }
}

TEST_F(StatComputerTest, test_expected_values_masked_channels) // NOLINT
{
  // This is gausian data with mean of 3.14, stddev of 10, rounded to nearest int
  // There are 4 channels, 2 pols, 2 dims, and 8 samples each (128 values)
  std::vector<int16_t> data = {
    // Pol A - channel 1
     -4,  19,  17,   6,  -2,   2,   0,  15,  15,   3,  15,   8, -11, -21, -18,   2,
    // Pol A - channel 2
    -11,   9,  -3,   5,  -4, -13,  12,  -1,   5,  10,  21,   0,  25,  -2,   0,  12,
    // Pol A - channel 3
      8,  -6,  -8,  23, -11,  -6,  28,   3,  32,  -2,  17,   6,  -8,   4,  -9,   0,
    // Pol A - channel 4
     12,   6,  -9, -18,  -5,   0, -12,   1,  12,   9, -18,   8,   9,   2,   0,  -8,
    // Pol B - channel 1
      4,   9,   0,  14,  24,   0,  17,   2,  -5,   0,   7,  11,   8,  -3,   2,  12,
    // Pol B - channel 2
      8,   8,  19,   3,  13,  22,  -2, -10, -13,  19,  -1,  16,  -2,   2,   0,  -3,
    // Pol B - channel 3
      1, -23,  -1,  32,   1,  15,   5,  10,  -1,  20,  -1,   6,  15, -13,  -4,   5,
    // Pol B - channel 4
     -1,   5,  -1,   1,  12,  -3,  -6,  -6,   0,  -5,  15,  12,  20,  13,  -2,  21
  };
  char * data_block = reinterpret_cast<char *>(data.data());
  float scale{1.0};

  ska::pst::common::AsciiHeader config;
  config.set("NDIM", 2);
  config.set("NPOL", 2);
  config.set("NBIT", 16);
  config.set("NCHAN", 4);
  config.set("NMASK", 1);
  // this will mask channel 1 & 2
  config.set("FREQ_MASK", "850:950");
  config.set("FREQ", 1000);
  config.set("BW", 400);
  config.set("UDP_NSAMP", 8);
  config.set("UDP_NCHAN", 4);
  config.set("WT_NSAMP", 8);
  config.set("PACKET_WEIGHTS_SIZE", 0);
  config.set("PACKET_SCALES_SIZE", 4);
  config.set("STAT_NREBIN", 1);

  configure(config, false);

  computer->compute(data_block, 256, reinterpret_cast<char *>(&scale), 4);

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
}

TEST_F(StatComputerTest, test_clipped_channels) // NOLINT
{
  // This is gausian data with mean of 3.14, stddev of 10, rounded to nearest int
  // There are 4 channels, 2 pols, 2 dims, and 8 samples each (128 values).
  // Some values have been put in a bin that should be masked
  std::vector<int16_t> data = {
    // Pol A - channel 1
    -32768,  19,  17,   6,  -2,   2,   0,  15,  15,   3,  15,   8, -11, -21, -18,   2,
    // Pol A - channel 2
    -11,   32767,  -3,   5,  -4, -13,  12,  -1,   5,  10,  21,   0,  25,  -2,   0,  12,
    // Pol A - channel 3
      8,  -6,  -32768,  23, -11,  -6,  28,   3,  32,  -2,  17,   6,  -8,   4,  -9,   0,
    // Pol A - channel 4
     12,   6,  -9, 32767,  -5,   0, -12,   1,  12,   9, -18,   8,   9,   2,   0,  -8,
    // Pol B - channel 1
      4,   9,   0,  14,  32767,   0,  17,   2,  -5,   0,   7,  11,   8,  -3,   2,  12,
    // Pol B - channel 2
      8,   8,  19,   3,  13,  32767,  -2, -10, -13,  19,  -1,  16,  -2,   2,   0,  -3,
    // Pol B - channel 3
      1, -23,  -1,  32,   1,  15,   32767,  10,  -1,  20,  -1,   6,  15, -13,  -4,   5,
    // Pol B - channel 4
     -1,   5,  -1,   1,  12,  -3,  -6,  -32768,   0,  -5,  15,  12,  20,  13,  -2,  21
  };
  char * data_block = reinterpret_cast<char *>(data.data());
  float scale{1.0};

  ska::pst::common::AsciiHeader config;
  config.set("NDIM", 2);
  config.set("NPOL", 2);
  config.set("NBIT", 16);
  config.set("NCHAN", 4);
  config.set("NMASK", 0);
  config.set("FREQ", 1000);
  config.set("BW", 400);
  config.set("UDP_NSAMP", 8);
  config.set("UDP_NCHAN", 4);
  config.set("WT_NSAMP", 8);
  config.set("PACKET_WEIGHTS_SIZE", 0);
  config.set("PACKET_SCALES_SIZE", 4);
  config.set("STAT_NREBIN", 1);

  configure(config, false);

  computer->compute(data_block, 256, reinterpret_cast<char *>(&scale), 4);

  ASSERT_EQ(storage->num_clipped_samples[0][0], 2);
  ASSERT_EQ(storage->num_clipped_samples[0][1], 2);
  ASSERT_EQ(storage->num_clipped_samples[1][0], 2);
  ASSERT_EQ(storage->num_clipped_samples[1][1], 2);

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

} // namespace ska::pst::stat::test
