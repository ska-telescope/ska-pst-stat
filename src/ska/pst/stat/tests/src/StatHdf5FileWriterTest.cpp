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

#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

#include "ska/pst/common/definitions.h"
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/Timer.h"
#include "ska/pst/stat/tests/StatHdf5FileWriterTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <H5Cpp.h>

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatHdf5FileWriterTest::StatHdf5FileWriterTest()
    : ::testing::Test()
{
}

void StatHdf5FileWriterTest::SetUp()
{
}

void StatHdf5FileWriterTest::TearDown()
{
}

void StatHdf5FileWriterTest::initialise(const std::string& config_file)
{
  config.load_from_file(test_data_file(config_file));
  storage = std::make_shared<StatStorage>(config);

  auto tsamp = config.get_double("TSAMP");
  auto nsamp_per_packet = config.get_uint64("UDP_NSAMP");
  auto total_sample_time = tsamp * ska::pst::common::seconds_per_microseconds * static_cast<double>(nsamp_per_packet);

  storage->set_total_sample_time(total_sample_time);

  auto ntime_bins = config.get_uint32("STAT_REQ_TIME_BINS");
  auto nfreq_bins = config.get_uint32("STAT_REQ_FREQ_BINS");
  storage->resize(ntime_bins, nfreq_bins);

  writer = std::make_shared<StatHdf5FileWriter>(config);

  populate_storage();
}

void StatHdf5FileWriterTest::populate_storage()
{
  // header values
  populate_1d_vec<double>(storage->channel_centre_frequencies);
  populate_1d_vec<double>(storage->timeseries_bins);
  populate_1d_vec<double>(storage->frequency_bins);

  // data
  populate_2d_vec<float>(storage->mean_frequency_avg);
  populate_2d_vec<float>(storage->mean_frequency_avg_rfi_excised);
  populate_2d_vec<float>(storage->variance_frequency_avg);
  populate_2d_vec<float>(storage->variance_frequency_avg_rfi_excised);
  populate_3d_vec<float>(storage->mean_spectrum);
  populate_3d_vec<float>(storage->variance_spectrum);
  populate_2d_vec<float>(storage->mean_spectral_power);
  populate_2d_vec<float>(storage->max_spectral_power);
  populate_3d_vec<uint32_t>(storage->histogram_1d_freq_avg);
  populate_3d_vec<uint32_t>(storage->histogram_1d_freq_avg_rfi_excised);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_2d_freq_avg);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_2d_freq_avg_rfi_excised);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_1d_freq_avg);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_1d_freq_avg_rfi_excised);
  populate_3d_vec<uint32_t>(storage->num_clipped_samples_spectrum);
  populate_2d_vec<uint32_t>(storage->num_clipped_samples);
  populate_2d_vec<uint32_t>(storage->num_clipped_samples_rfi_excised);
  populate_3d_vec<float>(storage->spectrogram);
  populate_3d_vec<float>(storage->timeseries);
  populate_3d_vec<float>(storage->timeseries_rfi_excised);
}

void StatHdf5FileWriterTest::validate_hdf5_file(const std::shared_ptr<H5::H5File>& file)
{
  std::array<hsize_t, 1> dims = { 1 };

  H5::DataSet dataset = file->openDataSet("FILE_FORMAT_VERSION");
  H5::DataSpace dataspace(H5S_SCALAR);
  H5::StrType str_datatype(H5::PredType::C_S1, H5T_VARIABLE);
  std::string file_format_version;
  dataset.read(file_format_version, str_datatype, dataspace);

  ASSERT_EQ(file_format_version, "1.0.0");

  auto header_datatype = writer->get_hdf5_header_datatype();

  dataset = file->openDataSet("HEADER");
  dataspace = dataset.getSpace();

  ASSERT_EQ(1, dataspace.getSimpleExtentNdims());
  dataspace.getSimpleExtentDims(dims.data());
  ASSERT_EQ(dims[0], 1);

  stat_hdf5_header_t header;
  dataset.read(&header, header_datatype);

  auto picoseconds = config.get_uint64("PICOSECONDS");
  auto t_min = static_cast<double>(ska::pst::common::attoseconds_per_microsecond) /
    static_cast<double>(ska::pst::common::attoseconds_per_second) *
    static_cast<double>(picoseconds);

  ASSERT_EQ(std::string(header.eb_id), config.get_val("EB_ID"));
  ASSERT_EQ(std::string(header.telescope), config.get_val("TELESCOPE"));
  ASSERT_EQ(header.scan_id, config.get_uint64("SCAN_ID"));
  ASSERT_EQ(std::string(header.beam_id), config.get_val("BEAM_ID"));
  ASSERT_EQ(std::string(header.utc_start), config.get_val("UTC_START"));
  ASSERT_EQ(header.t_min, t_min);
  ASSERT_EQ(header.t_max, t_min + storage->get_total_sample_time());
  ASSERT_EQ(header.freq, config.get_double("FREQ"));
  ASSERT_EQ(header.bandwidth, config.get_double("BW"));
  ASSERT_EQ(header.start_chan, config.get_uint32("START_CHANNEL"));
  ASSERT_EQ(header.npol, storage->get_npol());
  ASSERT_EQ(header.ndim, storage->get_ndim());
  ASSERT_EQ(header.nchan, storage->get_nchan());
  ASSERT_EQ(header.nbin, storage->get_nbin());
  ASSERT_EQ(header.nfreq_bins, storage->get_nfreq_bins());
  ASSERT_EQ(header.ntime_bins, storage->get_ntime_bins());
  ASSERT_EQ(header.nrebin, storage->get_nrebin());

  SPDLOG_DEBUG("About to assert chan_freq");

  // chan_freq is var length
  assert_1d_vec_header(storage->channel_centre_frequencies, header.chan_freq);
  assert_1d_vec_header(storage->timeseries_bins, header.timeseries_bins);
  assert_1d_vec_header(storage->frequency_bins, header.frequency_bins);

  assert_2d_vec<float>(storage->mean_frequency_avg, file, "MEAN_FREQUENCY_AVG", H5::PredType::NATIVE_FLOAT);
  assert_2d_vec<float>(storage->mean_frequency_avg_rfi_excised, file, "MEAN_FREQUENCY_AVG_RFI_EXCISED", H5::PredType::NATIVE_FLOAT);
  assert_2d_vec<float>(storage->variance_frequency_avg, file, "VARIANCE_FREQUENCY_AVG", H5::PredType::NATIVE_FLOAT);
  assert_2d_vec<float>(storage->variance_frequency_avg_rfi_excised, file, "VARIANCE_FREQUENCY_AVG_RFI_EXCISED", H5::PredType::NATIVE_FLOAT);
  assert_3d_vec<float>(storage->mean_spectrum, file, "MEAN_SPECTRUM", H5::PredType::NATIVE_FLOAT);
  assert_3d_vec<float>(storage->variance_spectrum, file, "VARIANCE_SPECTRUM", H5::PredType::NATIVE_FLOAT);
  assert_2d_vec<float>(storage->mean_spectral_power, file, "MEAN_SPECTRAL_POWER", H5::PredType::NATIVE_FLOAT);
  assert_2d_vec<float>(storage->max_spectral_power, file, "MAX_SPECTRAL_POWER", H5::PredType::NATIVE_FLOAT);
  assert_3d_vec<uint32_t>(storage->histogram_1d_freq_avg, file, "HISTOGRAM_1D_FREQ_AVG", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<uint32_t>(storage->histogram_1d_freq_avg_rfi_excised, file, "HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<uint32_t>(storage->rebinned_histogram_2d_freq_avg, file, "HISTOGRAM_REBINNED_2D_FREQ_AVG", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<uint32_t>(storage->rebinned_histogram_2d_freq_avg_rfi_excised, file, "HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<uint32_t>(storage->rebinned_histogram_1d_freq_avg, file, "HISTOGRAM_REBINNED_1D_FREQ_AVG", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<uint32_t>(storage->rebinned_histogram_1d_freq_avg_rfi_excised, file, "HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<uint32_t>(storage->num_clipped_samples_spectrum, file, "NUM_CLIPPED_SAMPLES_SPECTRUM", H5::PredType::NATIVE_UINT32);
  assert_2d_vec<uint32_t>(storage->num_clipped_samples, file, "NUM_CLIPPED_SAMPLES", H5::PredType::NATIVE_UINT32);
  assert_2d_vec<uint32_t>(storage->num_clipped_samples_rfi_excised, file, "NUM_CLIPPED_SAMPLES_RFI_EXCISED", H5::PredType::NATIVE_UINT32);
  assert_3d_vec<float>(storage->spectrogram, file, "SPECTROGRAM", H5::PredType::NATIVE_FLOAT);
  assert_3d_vec<float>(storage->timeseries, file, "TIMESERIES", H5::PredType::NATIVE_FLOAT);
  assert_3d_vec<float>(storage->timeseries_rfi_excised, file, "TIMESERIES_RFI_EXCISED", H5::PredType::NATIVE_FLOAT);
}

TEST_F(StatHdf5FileWriterTest, test_generates_correct_data) // NOLINT
{
  initialise();
  writer->publish(storage);

  std::shared_ptr<H5::H5File> file = std::make_shared<H5::H5File>(config.get_val("STAT_OUTPUT_FILENAME"), H5F_ACC_RDONLY);

  validate_hdf5_file(file);
}

} // namespace ska::pst::stat::test
