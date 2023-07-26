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

#include <spdlog/spdlog.h>
#include "ska/pst/stat/StatStorage.h"

ska::pst::stat::StatStorage::StatStorage(
  const ska::pst::common::AsciiHeader& config
)
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::StatStorage");
  this->config = config;
}

ska::pst::stat::StatStorage::~StatStorage()
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::~StatStorage");
}

void ska::pst::stat::StatStorage::reset()
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::reset");
  // 1d arrays
  std::fill(channel_centre_frequencies.begin(),channel_centre_frequencies.end(),0.0); // float
  std::fill(timeseries_bins.begin(),timeseries_bins.end(),0.0); // float
  std::fill(frequnecy_bins.begin(),frequnecy_bins.end(),0.0); // float

  // 2d arrays
  // Assumed header fields
  uint32_t npol = this->config.get_uint32("NPOL");
  uint32_t ndim = this->config.get_uint32("NDIM");
  uint32_t nchan = this->config.get_uint32("NCHAN");
  uint32_t nbit = this->config.get_uint32("NBIT");
  uint32_t nbin_rescaled = this->config.get_uint32("NBIN_RESCALED");

  for(uint32_t ipol=0; ipol<npol; ipol++)
  {
    std::fill(mean_frequency_avg[ipol].begin(), mean_frequency_avg[ipol].end(), 0.0);
    std::fill(mean_frequency_avg_masked[ipol].begin(), mean_frequency_avg_masked[ipol].end(), 0.0);
    std::fill(variance_frequency_avg[ipol].begin(), variance_frequency_avg[ipol].end(), 0.0);
    std::fill(variance_frequency_avg_masked[ipol].begin(), variance_frequency_avg_masked[ipol].end(), 0.0);
    std::fill(mean_spectral_power[ipol].begin(), mean_spectral_power[ipol].end(), 0.0);
    std::fill(max_spectral_power[ipol].begin(), max_spectral_power[ipol].end(), 0.0);
    std::fill(num_clipped_samples[ipol].begin(), num_clipped_samples[ipol].end(), 0.0);
    for(uint32_t idim=0; idim<ndim; idim++)
    {
      std::fill(mean_spectrum[ipol][idim].begin(), mean_spectrum[ipol][idim].end(), 0);
      std::fill(variance_spectrum[ipol][idim].begin(), variance_spectrum[ipol][idim].end(), 0);
      std::fill(histogram_1d_freq_avg[ipol][idim].begin(), histogram_1d_freq_avg[ipol][idim].end(), 0);
      std::fill(histogram_1d_freq_avg_masked[ipol][idim].begin(), histogram_1d_freq_avg_masked[ipol][idim].end(), 0);
      std::fill(rebinned_histogram_2d_freq_avg[ipol][idim].begin(), rebinned_histogram_2d_freq_avg[ipol][idim].end(), 0);
      std::fill(rebinned_histogram_1d_freq_avg[ipol][idim].begin(), rebinned_histogram_1d_freq_avg[ipol][idim].end(), 0);
      std::fill(rebinned_histogram_1d_freq_avg_masked[ipol][idim].begin(), rebinned_histogram_1d_freq_avg_masked[ipol][idim].end(), 0);
      std::fill(num_clipped_samples_spectrum[ipol][idim].begin(), num_clipped_samples_spectrum[ipol][idim].end(), 0);
      std::fill(spectrogram[ipol][idim].begin(), spectrogram[ipol][idim].end(), 0);
      std::fill(timeseries[ipol][idim].begin(), timeseries[ipol][idim].end(), 0);
      std::fill(timeseries_masked[ipol][idim].begin(), timeseries_masked[ipol][idim].end(), 0);
    }
  }


}

void ska::pst::stat::StatStorage::resize(uint32_t ntime_bins, uint32_t nfreq_bins)
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::resize ntime_bins={} nfreq_bins={}", ntime_bins, nfreq_bins);
}