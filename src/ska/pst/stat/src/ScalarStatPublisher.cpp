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
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "ska/pst/common/definitions.h"
#include "ska/pst/stat/ScalarStatPublisher.h"


ska::pst::stat::ScalarStatPublisher::ScalarStatPublisher(
  const ska::pst::common::AsciiHeader& config,
  std::shared_ptr<StatStorage> storage
) : StatPublisher(config, std::move(storage))
{
  SPDLOG_DEBUG("ska::pst::stat::ScalarStatPublisher::ScalarStatPublisher");
}

ska::pst::stat::ScalarStatPublisher::~ScalarStatPublisher()
{
  SPDLOG_DEBUG("ska::pst::stat::ScalarStatPublisher::~ScalarStatPublisher()");
}

void ska::pst::stat::ScalarStatPublisher::publish()
{
  SPDLOG_DEBUG("ska::pst::stat::ScalarStatPublisher::publish()");
  std::lock_guard<std::mutex> lock(scalar_stats_mutex);
  scalar_stats.mean_frequency_avg = storage->mean_frequency_avg;
  scalar_stats.mean_frequency_avg_masked = storage->mean_frequency_avg_masked;
  scalar_stats.variance_frequency_avg = storage->variance_frequency_avg;
  scalar_stats.variance_frequency_avg_masked = storage->variance_frequency_avg_masked;
  scalar_stats.num_clipped_samples_spectrum = storage->num_clipped_samples_spectrum;
  scalar_stats.num_clipped_samples = storage->num_clipped_samples;
}

auto ska::pst::stat::ScalarStatPublisher::get_scalar_stats() -> ska::pst::stat::StatStorage::scalar_stats_t
{
  SPDLOG_DEBUG("ska::pst::stat::ScalarStatPublisher::get_scalar_stats()");
  std::lock_guard<std::mutex> lock(scalar_stats_mutex);
  return scalar_stats;
}
