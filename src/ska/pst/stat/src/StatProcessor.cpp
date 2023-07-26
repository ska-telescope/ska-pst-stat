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
}

ska::pst::stat::StatProcessor::~StatProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::~StatProcessor()");
}

void ska::pst::stat::StatProcessor::process(
    char * data_block,
    size_t block_length,
    char * weights,
    size_t weights_length)
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process data_block={}", data_block);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process block_length={}", block_length);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process weights={}", weights);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process weights_length={}", weights_length);

  // need to determine a few parameters in the storage.
  // TODO: confirm the source of nbytes_per_sample desired_time_bins desired_freq_bins nchan
  uint32_t nbytes_per_sample = 0;
  uint32_t desired_time_bins = 0;
  uint32_t desired_freq_bins = 0;
  uint32_t nchan = 0;

  uint32_t nsamp_block = block_length / nbytes_per_sample;
  if (block_length % nbytes_per_sample != 0)
  {
    throw std::current_exception();
  }

  uint32_t ntime_bins = nsamp_block;
  while (ntime_bins > desired_time_bins)
  {
    ntime_bins /= 2;
  }

  uint32_t nfreq_bins = nchan;
  while (nfreq_bins > desired_freq_bins)
  {
    nfreq_bins /= 2;
  }

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process storage->resize({}, {})", ntime_bins, nfreq_bins);
  storage->resize(ntime_bins, nfreq_bins);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process storage->reset()");
  storage->reset();

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process computer->process()");
  computer->compute(data_block, block_length, weights, weights_length);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process publisher->publish()");
  publisher->publish();
}