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
#include <cstdlib>
#include <cstring>
#include <H5Cpp.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "ska/pst/stat/StatHdf5FileWriter.h"
#include "ska/pst/common/definitions.h"


ska::pst::stat::StatHdf5FileWriter::StatHdf5FileWriter(
  const ska::pst::common::AsciiHeader& config,
  std::shared_ptr<StatStorage> storage
) : StatPublisher(config, std::move(storage))
{
  SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::StatHdf5FileWriter");
}

ska::pst::stat::StatHdf5FileWriter::~StatHdf5FileWriter()
{
  SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::~StatHdf5FileWriter()");
}

auto ska::pst::stat::StatHdf5FileWriter::get_hdf5_header_datatype() -> H5::CompType
{
  H5::StrType str_datatype(H5::PredType::C_S1, H5T_VARIABLE);

  H5::CompType header_datatype(sizeof(stat_hdf5_header_t));
  header_datatype.insertMember("EB_ID", HOFFSET(stat_hdf5_header_t, eb_id), str_datatype);
  header_datatype.insertMember("SCAN_ID", HOFFSET(stat_hdf5_header_t, scan_id), H5::PredType::NATIVE_UINT64);
  header_datatype.insertMember("BEAM_ID", HOFFSET(stat_hdf5_header_t, beam_id), str_datatype);
  header_datatype.insertMember("UTC_START", HOFFSET(stat_hdf5_header_t, utc_start), str_datatype);
  header_datatype.insertMember("T_MIN", HOFFSET(stat_hdf5_header_t, t_min), H5::PredType::NATIVE_DOUBLE);
  header_datatype.insertMember("T_MAX", HOFFSET(stat_hdf5_header_t, t_max), H5::PredType::NATIVE_DOUBLE);
  header_datatype.insertMember("FREQ", HOFFSET(stat_hdf5_header_t, freq), H5::PredType::NATIVE_DOUBLE);
  header_datatype.insertMember("BW", HOFFSET(stat_hdf5_header_t, bandwidth), H5::PredType::NATIVE_DOUBLE);
  header_datatype.insertMember("START_CHAN", HOFFSET(stat_hdf5_header_t, start_chan), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NPOL", HOFFSET(stat_hdf5_header_t, npol), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NDIM", HOFFSET(stat_hdf5_header_t, ndim), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NCHAN", HOFFSET(stat_hdf5_header_t, nchan), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NCHAN_DS", HOFFSET(stat_hdf5_header_t, nfreq_bins), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NDAT_DS", HOFFSET(stat_hdf5_header_t, ntime_bins), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NBIN_HIST", HOFFSET(stat_hdf5_header_t, nbin), H5::PredType::NATIVE_UINT32);
  header_datatype.insertMember("NREBIN", HOFFSET(stat_hdf5_header_t, nrebin), H5::PredType::NATIVE_UINT32);

  // header binning data
  header_datatype.insertMember("CHAN_FREQ", HOFFSET(stat_hdf5_header_t, chan_freq), H5::VarLenType(H5::PredType::NATIVE_DOUBLE));
  header_datatype.insertMember("FREQUENCY_BINS", HOFFSET(stat_hdf5_header_t, frequency_bins), H5::VarLenType(H5::PredType::NATIVE_DOUBLE));
  header_datatype.insertMember("TIMESERIES_BINS", HOFFSET(stat_hdf5_header_t, timeseries_bins), H5::VarLenType(H5::PredType::NATIVE_DOUBLE));

  return header_datatype;
}

void ska::pst::stat::StatHdf5FileWriter::publish()
{
  SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::publish()");
  SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::publish() - config\n{}", config.raw());

  auto npol = storage->get_npol();
  auto ndim = storage->get_ndim();
  auto nchan = storage->get_nchan();
  auto nbin = storage->get_nbin();
  auto nfreq_bins = storage->get_nfreq_bins();
  auto ntime_bins = storage->get_ntime_bins();
  auto nrebin = storage->get_nrebin();

  stat_hdf5_header_t header;

  auto picoseconds = config.get_uint64("PICOSECONDS");
  auto t_min = static_cast<double>(ska::pst::common::attoseconds_per_microsecond) /
    static_cast<double>(ska::pst::common::attoseconds_per_second) *
    static_cast<double>(picoseconds);

  std::string eb_id = config.get_val("EB_ID");
  std::string beam_id = config.get_val("BEAM_ID");
  std::string utc_start = config.get_val("UTC_START");

  header.eb_id = const_cast<char *>(eb_id.c_str()); // NOLINT
  header.scan_id = config.get_uint64("SCAN_ID");
  header.beam_id = const_cast<char *>(beam_id.c_str()); // NOLINT
  header.utc_start = const_cast<char *>(utc_start.c_str()); // NOLINT
  header.t_max = t_min + storage->get_total_sample_time();
  header.t_min = t_min;
  header.freq = config.get_double("FREQ");
  header.bandwidth = config.get_double("BW");
  header.start_chan = config.get_uint32("START_CHAN");
  header.npol = npol;
  header.ndim = ndim;
  header.nchan = nchan;
  header.nbin = nbin;
  header.nfreq_bins = nfreq_bins;
  header.ntime_bins = ntime_bins;
  header.nrebin = nrebin;

  write_1d_vec_header(storage->channel_centre_frequencies, header.chan_freq);
  write_1d_vec_header(storage->frequency_bins, header.frequency_bins);
  write_1d_vec_header(storage->timeseries_bins, header.timeseries_bins);

  std::vector<char> temp_data;

  file = std::make_shared<H5::H5File>(config.get_val("STAT_OUTPUT_FILENAME"), H5F_ACC_TRUNC);

  auto header_datatype = get_hdf5_header_datatype();

  // Define the dataset dimensions
  const int rank = 1;
  std::array<hsize_t, rank> dims = {1};

  H5::DataSpace header_dataspace(rank, &dims[0]);
  H5::DataSet header_dataset = file->createDataSet("HEADER", header_datatype, header_dataspace);
  header_dataset.write(&header, header_datatype);

  write_2d_vec<float>(storage->mean_frequency_avg, "MEAN_FREQUENCY_AVG", H5::PredType::NATIVE_FLOAT, temp_data);
  write_2d_vec<float>(storage->mean_frequency_avg_masked, "MEAN_FREQUENCY_AVG_MASKED", H5::PredType::NATIVE_FLOAT, temp_data);
  write_2d_vec<float>(storage->variance_frequency_avg, "VARIANCE_FREQUENCY_AVG", H5::PredType::NATIVE_FLOAT, temp_data);
  write_2d_vec<float>(storage->variance_frequency_avg_masked, "VARIANCE_FREQUENCY_AVG_MASKED", H5::PredType::NATIVE_FLOAT, temp_data);
  write_3d_vec<float>(storage->mean_spectrum, "MEAN_SPECTRUM", H5::PredType::NATIVE_FLOAT, temp_data);
  write_3d_vec<float>(storage->variance_spectrum, "VARIANCE_SPECTRUM", H5::PredType::NATIVE_FLOAT, temp_data);
  write_2d_vec<float>(storage->mean_spectral_power, "MEAN_SPECTRAL_POWER", H5::PredType::NATIVE_FLOAT, temp_data);
  write_2d_vec<float>(storage->max_spectral_power, "MAX_SPECTRAL_POWER", H5::PredType::NATIVE_FLOAT, temp_data);
  write_3d_vec<uint32_t>(storage->histogram_1d_freq_avg, "HISTOGRAM_1D_FREQ_AVG", H5::PredType::NATIVE_UINT32, temp_data);
  write_3d_vec<uint32_t>(storage->histogram_1d_freq_avg_masked, "HISTOGRAM_1D_FREQ_AVG_MASKED", H5::PredType::NATIVE_UINT32, temp_data);
  write_3d_vec<uint32_t>(storage->num_clipped_samples_spectrum, "NUM_CLIPPED_SAMPLES_SPECTRUM", H5::PredType::NATIVE_UINT32, temp_data);
  write_2d_vec<uint32_t>(storage->num_clipped_samples, "NUM_CLIPPED_SAMPLES", H5::PredType::NATIVE_UINT32, temp_data);
  write_3d_vec<float>(storage->spectrogram, "SPECTROGRAM", H5::PredType::NATIVE_FLOAT, temp_data);
  write_3d_vec<float>(storage->timeseries, "TIMESERIES", H5::PredType::NATIVE_FLOAT, temp_data);
  write_3d_vec<float>(storage->timeseries_masked, "TIMESERIES_MASKED", H5::PredType::NATIVE_FLOAT, temp_data);

  file->close();
}

void ska::pst::stat::StatHdf5FileWriter::write_array(
  const std::vector<char>& data,
  const std::string& field_name,
  const H5::PredType& datatype,
  H5::DataSpace& dataspace
)
{
  H5::DataSet dataset = file->createDataSet(field_name, datatype, dataspace);
  dataset.write(data.data(), datatype);
  dataset.close();
}
