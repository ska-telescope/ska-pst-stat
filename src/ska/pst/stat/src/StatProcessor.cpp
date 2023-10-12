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
#include <filesystem>

#include "ska/pst/stat/StatProcessor.h"
#include "ska/pst/stat/StatHdf5FileWriter.h"

ska::pst::stat::StatProcessor::StatProcessor(
  const ska::pst::common::AsciiHeader& _data_config,
  const ska::pst::common::AsciiHeader& _weights_config
) : data_config(_data_config), weights_config(_weights_config), req_time_bins(default_ntime_bins), req_freq_bins(default_nfreq_bins)

{
  data_resolution = data_config.get_uint32("RESOLUTION");
  weights_resolution = weights_config.get_uint32("RESOLUTION");
  if (data_config.has("STAT_REQ_TIME_BINS"))
  {
    req_time_bins = data_config.get_uint32("STAT_REQ_TIME_BINS");
  }
  if (data_config.has("STAT_REQ_FREQ_BINS"))
  {
    req_freq_bins = data_config.get_uint32("STAT_REQ_FREQ_BINS");
  }
  nchan = data_config.get_uint32("NCHAN");

  if (req_time_bins <= 0 || req_time_bins > max_time_bins)
  {
    SPDLOG_WARN("ska::pst::stat::StatProcessor::StatProcessor req_time_bins={}", req_time_bins);
    req_time_bins = default_ntime_bins;
    SPDLOG_INFO("ska::pst::stat::StatProcessor::StatProcessor req_time_bins set to {}", req_time_bins);
  }

  if (req_freq_bins <= 0 || req_freq_bins > max_freq_bins)
  {
    SPDLOG_WARN("ska::pst::stat::StatProcessor::StatProcessor req_freq_bins={}", req_freq_bins);
    req_freq_bins = default_nfreq_bins;
    SPDLOG_INFO("ska::pst::stat::StatProcessor::StatProcessor req_freq_bins set to {}", req_freq_bins);
  }

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::StatProcessor create new shared StatStorage object");
  storage = std::make_shared<ska::pst::stat::StatStorage>(data_config);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::StatProcessor create new unique StatComputer object");
  computer = std::make_unique<ska::pst::stat::StatComputer>(data_config, weights_config, storage);
}

ska::pst::stat::StatProcessor::~StatProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::~StatProcessor()");
}

void ska::pst::stat::StatProcessor::interrupt()
{
  computer->interrupt();
}

void ska::pst::stat::StatProcessor::add_publisher(std::unique_ptr<StatPublisher> publisher)
{
  publishers.push_back(std::move(publisher));
}

auto ska::pst::stat::StatProcessor::process(const ska::pst::common::SegmentProducer::Segment& segment) -> bool
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process segment.data.size={}", segment.data.size);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process segment.weights.size={}", segment.weights.size);

  if (segment.data.block == nullptr)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process segment.data.block pointer is null");
    throw std::runtime_error("ska::pst::stat::StatProcessor::process segment.data.block pointer is null");
  }
  if (segment.weights.block == nullptr)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process segment.weights.block pointer is null");
    throw std::runtime_error("ska::pst::stat::StatProcessor::process segment.weights.block pointer is null");
  }

  // need to determine a few parameters in the storage.
  uint32_t nbytes_per_sample = data_config.compute_bits_per_sample() / ska::pst::common::bits_per_byte;

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nbytes_per_sample={}", nbytes_per_sample);
  if (nbytes_per_sample == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process nbytes_per_sample equals 0");
    throw std::runtime_error("ska::pst::stat::StatProcessor::process nbytes_per_sample equals 0");
  }

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process segment.data.size={}", segment.data.size);
  if (segment.data.size == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process segment.data.size equals 0");
    throw std::runtime_error("ska::pst::stat::StatProcessor::process segment.data.size equals 0");
  }

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process segment.weights.size={}", segment.weights.size);
  if (segment.weights.size == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process segment.weights.size equals 0");
    throw std::runtime_error("ska::pst::stat::StatProcessor::process segment.weights.size equals 0");
  }

  if (data_resolution % nbytes_per_sample != 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process data_resolution \% nbytes_per_sample={}", (data_resolution % nbytes_per_sample));
    throw std::runtime_error("ska::pst::stat::StatProcessor::process data resolution not a multiple of nbytes_per_sample");
  }

  auto num_data_heaps = segment.data.size / data_resolution;
  auto num_weights_heaps = segment.weights.size / weights_resolution;
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process num_data_heaps={}", num_data_heaps);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process num_weights_heaps={}", num_weights_heaps);
  if (num_data_heaps == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process segment.data.size \% data_resolution={}", (segment.data.size % data_resolution));
    throw std::runtime_error("ska::pst::stat::StatProcessor::process block length of data to process is effectively 0 bytes");
  }

  if (num_weights_heaps == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process segment.weights.size \% weights_resolution={}", (segment.weights.size % weights_resolution));
    throw std::runtime_error("ska::pst::stat::StatProcessor::process block length of weights to process is effectively 0 bytes");
  }

  if (num_data_heaps != num_weights_heaps)
  {
    SPDLOG_ERROR("ska::pst::stat::StatProcessor::process num_data_heaps={}, num_weights_heaps={} are not equal", num_data_heaps, num_weights_heaps);
    throw std::runtime_error("ska::pst::stat::StatProcessor::process number of heaps in the data and weights is not the same");
  }

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process segment.data.size \% data_resolution={}", (segment.data.size % data_resolution));
  if (segment.data.size % data_resolution != 0)
  {
    SPDLOG_WARN("ska::pst::stat::StatProcessor::process segment.data.size={} is not a multiple of data_resolution={} effectively using only {} bytes",
      segment.data.size, data_resolution, (num_data_heaps * data_resolution));
  }

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process segment.weights.size \% data_resolution={}", (segment.weights.size % weights_resolution));
  if (segment.weights.size % weights_resolution != 0)
  {
    SPDLOG_WARN("ska::pst::stat::StatProcessor::process segment.weights.size={} is not a multiple of weights_resolution={} effectively using only {} bytes",
      segment.weights.size, weights_resolution, (num_weights_heaps * weights_resolution));
  }

  uint32_t nsamp_block = num_data_heaps * data_resolution / nbytes_per_sample;
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nsamp_block={}", nsamp_block);

  uint32_t ntime_bins = calc_bins(nsamp_block, req_time_bins);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process ntime_bins={}", ntime_bins);

  uint32_t nfreq_bins = calc_bins(nchan, req_freq_bins);
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process nfreq_bins={}", nfreq_bins);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process storage->resize({}, {})", ntime_bins, nfreq_bins);
  storage->resize(ntime_bins, nfreq_bins);

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process computer->initialise()");
  computer->initialise();

  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process computer->process()");
  bool processing_complete = computer->compute(segment);

  if (processing_complete)
  {
    SPDLOG_DEBUG("ska::pst::stat::StatProcessor::process publishers->publish()");
    for (auto &publisher : publishers)
    {
      publisher->publish(storage);
    }
  }
  return processing_complete;
}

auto ska::pst::stat::StatProcessor::calc_bins(uint32_t block_length, uint32_t req_bins) -> uint32_t
{
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::calc_bins block_length={}, req_bins={}", block_length, req_bins);
  if (block_length % req_bins == 0) {
    SPDLOG_DEBUG("ska::pst::stat::StatProcessor::calc_bins block_length is factor of req_bins. Using {} bins", req_bins);
    return req_bins;
  }

  uint32_t estimate_nbin_factor = std::max(block_length / req_bins, static_cast<uint32_t>(1));
  for (uint32_t nbin_factor = estimate_nbin_factor; nbin_factor > 1; nbin_factor--)
  {
    if (block_length % nbin_factor == 0)
    {
      uint32_t nbins = block_length / nbin_factor;
      SPDLOG_DEBUG("ska::pst::stat::StatProcessor::calc_bins nbins={}", nbins);
      return nbins;
    }
  }

  // at worse use the block_length as this has a nbin_factor of 1
  SPDLOG_DEBUG("ska::pst::stat::StatProcessor::calc_bins unable to find a factor close to {} using {} bins", req_bins, block_length);
  return block_length;
}
