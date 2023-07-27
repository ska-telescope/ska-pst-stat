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
#include <spdlog/spdlog.h>

#include "ska/pst/stat/StatProcessor.h"
#include "ska/pst/stat/StatHdf5FileWriter.h"

/*
TODO: implement the following architectural change
- split config involving data and weights
*/
ska::pst::stat::StatProcessor::StatProcessor(
  const ska::pst::common::AsciiHeader& config
)
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::StatProcessor");
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::StatProcessor create new shared StatStorage object");
  storage=std::make_shared<ska::pst::stat::StatStorage>(config);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::StatProcessor create new unique StatComputer object");
  computer=std::make_unique<ska::pst::stat::StatComputer>(config, storage);

  // Create StatHdf5FileWriter instance
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::StatProcessor create new unique StatHdf5FileWriter object");
  // TODO: Confirm the source of file_path
  const std::string& file_path = config.get_val("STAT_OUTPUT_BASEPATH");
  publisher=std::make_unique<ska::pst::stat::StatHdf5FileWriter>(config, storage, file_path);

  data_resolution = config.get_uint32("RESOLUTION");
  req_time_bins = config.get_uint32("STAT_REQ_TIME_BINS");
  req_freq_bins = config.get_uint32("STAT_REQ_FREQ_BINS");

  if (req_time_bins == 0 || req_time_bins > max_time_bins)
  {
    // create a warning
    SPDLOG_WARN("ska::pst::stat::StatProcessor::StatProcessor req_time_bins={}", req_time_bins);
    req_time_bins = 1024;
    SPDLOG_INFO("ska::pst::stat::StatProcessor::StatProcessor req_time_bins set to {}", req_time_bins);
  }

  if (req_freq_bins == 0 || req_freq_bins > max_freq_bins)
  {
    // create a warning
    SPDLOG_WARN("ska::pst::stat::StatProcessor::StatProcessor req_freq_bins={}", req_freq_bins);
    req_freq_bins = 1024;
    SPDLOG_INFO("ska::pst::stat::StatProcessor::StatProcessor req_freq_bins set to {}", req_freq_bins);
  }

  this->config=config;
}

ska::pst::stat::StatProcessor::~StatProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::~StatProcessor()");
}

/*
TODO: implement the following architectural change
- split processing involving data and weights
*/
void ska::pst::stat::StatProcessor::process(
    char * data_block,
    size_t data_length,
    char * weights_block,
    size_t weights_length
)
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process data_length={}", data_length);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process weights_length={}", weights_length);

  // need to determine a few parameters in the storage.  
  uint32_t nbytes_per_sample = config.compute_bits_per_sample() / ska::pst::common::bits_per_byte;
  uint32_t desired_time_bins = config.get_uint32("STAT_REQ_TIME_BINS"); // storage->get_ntime_bins();
  uint32_t desired_freq_bins = config.get_uint32("STAT_REQ_FREQ_BINS"); // storage->get_nfreq_bins();
  uint32_t nchan = config.get_uint32("NCHAN");
  
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nbytes_per_sample={}", nbytes_per_sample);
  if (nbytes_per_sample <= 0)
  {
    throw std::runtime_error("ska::pst::stat::StatProcessor::process nbytes_per_sample not greater than 0");
  }
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process desired_time_bins={}", desired_time_bins);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process desired_freq_bins={}", desired_freq_bins);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nchan={}", nchan);

  if (data_length == 0 || nbytes_per_sample == 0)
  {

    throw std::runtime_error(std::string("ska::pst::stat::StatProcessor::process data_length={} nbytes_per_sample={}", data_length, nbytes_per_sample));
  }

  uint32_t nsamp_block = data_length / nbytes_per_sample;
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nsamp_block={}", nsamp_block);
  if (data_resolution % nbytes_per_sample != 0)
  {
    SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process data_resolution \% nbytes_per_sample={}", (data_resolution % nbytes_per_sample));
    throw std::runtime_error("ska::pst::stat::StatProcessor::process data resolution not a multiple of nbytes_per_sample");
  }

  if (data_length % data_resolution != 0)
  {
    // throw an error
    SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process data_length \% data_resolution={}", (data_length % data_resolution));
    throw std::runtime_error("ska::pst::stat::StatProcessor::process block length not a multiple of data_resolution");
  }

  uint32_t ntime_factor = std::max(nsamp_block / req_time_bins, uint32_t(1));
  uint32_t ntime_bins = nsamp_block / ntime_factor;
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process ntime_bins={}", ntime_bins);

  uint32_t nfreq_factor = std::max(nchan / req_freq_bins, uint32_t(1));
  uint32_t nfreq_bins = nchan / nfreq_factor;
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nfreq_bins={}", nfreq_bins);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process storage->resize({}, {})", ntime_bins, nfreq_bins);
  storage->resize(ntime_bins, nfreq_bins);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process computer->process()");
  computer->compute(data_block, data_length, weights_block, weights_length);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process publisher->publish()");
  publisher->publish();
}