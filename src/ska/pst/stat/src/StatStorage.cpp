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
#include <stdexcept>
#include <string>
#include <vector>

#include "ska/pst/stat/StatStorage.h"

ska::pst::stat::StatStorage::StatStorage(const ska::pst::common::AsciiHeader& config)
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::StatStorage");

  npol = config.get_uint32("NPOL");
  ndim = config.get_uint32("NDIM");
  nchan = config.get_uint32("NCHAN");
  nbin = static_cast<uint32_t>(round(pow(2.0, config.get_double("NBIT"))));
  nrebin = config.get_uint32("STAT_NREBIN");
}

ska::pst::stat::StatStorage::~StatStorage()
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::~StatStorage()");
}

void ska::pst::stat::StatStorage::resize(uint32_t _ntime_bins, uint32_t _nfreq_bins)
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::resize ntime_bins={} nfreq_bins={}", _ntime_bins, _nfreq_bins);
  ntime_bins = _ntime_bins;
  nfreq_bins = _nfreq_bins;

  resize_1d(channel_centre_frequencies, nchan);

  resize_2d(mean_frequency_avg, npol, ndim);
  resize_2d(mean_frequency_avg_masked, npol, ndim);
  resize_2d(variance_frequency_avg, npol, ndim);
  resize_2d(variance_frequency_avg_masked, npol, ndim);

  resize_3d(mean_spectrum, npol, ndim, nchan);
  resize_3d(variance_spectrum, npol, ndim, nchan);

  resize_2d(mean_spectral_power, npol, nchan);
  resize_2d(max_spectral_power, npol, nchan);

  resize_3d(histogram_1d_freq_avg, npol, ndim, nbin);
  resize_3d(histogram_1d_freq_avg_masked, npol, ndim, nbin);

  resize_3d(rebinned_histogram_2d_freq_avg, npol, nrebin, nrebin);
  resize_3d(rebinned_histogram_2d_freq_avg_masked, npol, nrebin, nrebin);

  resize_3d(rebinned_histogram_1d_freq_avg, ndim, nrebin, nrebin);
  resize_3d(rebinned_histogram_1d_freq_avg_masked, npol, ndim, nrebin);

  resize_3d(num_clipped_samples_spectrum, npol, ndim, nchan);
  resize_2d(num_clipped_samples, npol, ndim);

  resize_1d(timeseries_bins, ntime_bins);
  resize_1d(frequency_bins, nfreq_bins);

  resize_3d(spectrogram, npol, nfreq_bins, ntime_bins);
  resize_3d(timeseries, npol, ntime_bins, ntime_vals);
  resize_3d(timeseries_masked, npol, ntime_bins, ntime_vals);

  resize_1d(rfi_mask_lut, nchan);

  SPDLOG_DEBUG("ska::pst::stat::StatStorage::resize resized=true");
  storage_resized = true;
}

void ska::pst::stat::StatStorage::reset()
{
  SPDLOG_DEBUG("ska::pst::stat::StatStorage::reset()");

  reset_1d(channel_centre_frequencies);

  reset_2d(mean_frequency_avg);
  reset_2d(mean_frequency_avg_masked);
  reset_2d(variance_frequency_avg);
  reset_2d(variance_frequency_avg_masked);

  reset_3d(mean_spectrum);
  reset_3d(variance_spectrum);

  reset_2d(mean_spectral_power);
  reset_2d(max_spectral_power);

  reset_3d(histogram_1d_freq_avg);
  reset_3d(histogram_1d_freq_avg_masked);

  reset_3d(rebinned_histogram_2d_freq_avg);
  reset_3d(rebinned_histogram_2d_freq_avg_masked);

  reset_3d(rebinned_histogram_1d_freq_avg);
  reset_3d(rebinned_histogram_1d_freq_avg_masked);

  reset_3d(num_clipped_samples_spectrum);
  reset_2d(num_clipped_samples);

  reset_1d(timeseries_bins);
  reset_1d(frequency_bins);

  reset_3d(spectrogram);
  reset_3d(timeseries);
  reset_3d(timeseries_masked);

  reset_1d(rfi_mask_lut);

  SPDLOG_DEBUG("ska::pst::stat::StatStorage::reset storage_reset=true");
  storage_reset = true;
}