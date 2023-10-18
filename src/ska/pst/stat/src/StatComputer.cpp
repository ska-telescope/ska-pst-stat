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

#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>
#include <limits>

#include "ska/pst/stat/StatComputer.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/common/definitions.h"

//! Index of the real/I part of the voltage
static constexpr uint32_t I_IDX = 0;
//! Index of the imaginary/Q part of the voltage
static constexpr uint32_t Q_IDX = 1;

//! Index for the max bin in timeseries data
static constexpr uint32_t TS_MAX_IDX = 0;
//! Index for the min bin in timeseries data
static constexpr uint32_t TS_MIN_IDX = 1;
//! Index for the mean bin in timeseries data
static constexpr uint32_t TS_MEAN_IDX = 2;

//! Constant of 1/2 factored out
static constexpr float half = 1.0/2.0;

ska::pst::stat::StatComputer::StatComputer(
  const ska::pst::common::AsciiHeader& _data_config,
  const ska::pst::common::AsciiHeader& _weights_config,
  std::shared_ptr<StatStorage> storage
) : data_config(_data_config), weights_config(_weights_config), storage(std::move(storage)) {
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer data_config:\n{}", data_config.raw());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer weights_config:\n{}", weights_config.raw());

  ndim = data_config.get_uint32("NDIM");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - ndim={}", ndim);
  if (ndim == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatComputer::StatComputer ndim not greater than 0");
    throw std::runtime_error("ska::pst::stat::StatComputer::StatComputer ndim not greater than 0");
  }

  npol = data_config.get_uint32("NPOL");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - npol={}", npol);
  if (npol == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatComputer::StatComputer npol not greater than 0");
    throw std::runtime_error("ska::pst::stat::StatComputer::StatComputer npol not greater than 0");
  }

  nbit = data_config.get_uint32("NBIT");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nbit={}", nbit);
  if (nbit == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatComputer::StatComputer nbit not greater than 0");
    throw std::runtime_error("ska::pst::stat::StatComputer::StatComputer nbit not greater than 0");
  }

  nchan = data_config.get_uint32("NCHAN");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nchan={}", nchan);
  if (nchan == 0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatComputer::StatComputer nchan not greater than 0");
    throw std::runtime_error("ska::pst::stat::StatComputer::StatComputer nchan not greater than 0");
  }

  tsamp = data_config.get_double("TSAMP");
  if (tsamp == 0.0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatComputer::StatComputer tsamp not greater than 0");
    throw std::runtime_error("ska::pst::stat::StatComputer::StatComputer tsamp not greater than 0");
  }

  bytes_per_second = data_config.get_double("BYTES_PER_SECOND");
  if (bytes_per_second == 0.0)
  {
    SPDLOG_ERROR("ska::pst::stat::StatComputer::StatComputer bytes_per_second not greater than 0");
    throw std::runtime_error("ska::pst::stat::StatComputer::StatComputer bytes_per_second not greater than 0");
  }

  if (data_config.has("NMASK")) {
    nmask = data_config.get_uint32("NMASK");
  }
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nmask={}", nmask);

  heap_layout.configure(data_config, weights_config);

  // optimization: avoid calling HeapLayout::get_weights_packet_stride in StatComputer::get_scale_factor
  weights_packet_stride = heap_layout.get_weights_packet_stride();

  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nsamp_per_packet={}", heap_layout.get_packet_layout().get_samples_per_packet());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nchan_per_packet={}", heap_layout.get_packet_layout().get_nchan_per_packet());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nsamp_per_weight={}", heap_layout.get_packet_layout().get_nsamp_per_weight());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - weights_packet_stride={}", heap_layout.get_weights_packet_stride());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - packet_resolution={}", heap_layout.get_data_packet_stride());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - data_heap_stride={}", heap_layout.get_data_heap_stride());
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - packets_per_heap={}", heap_layout.get_packets_per_heap());
}

ska::pst::stat::StatComputer::~StatComputer()
{
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::~StatComputer()");
}

void ska::pst::stat::StatComputer::initialise()
{
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise()");

  // calculate the channel_centre_frequencies
  auto bandwidth = data_config.get_double("BW");
  auto centre_freq = data_config.get_double("FREQ");
  auto channel_bandwidth = bandwidth / static_cast<double>(nchan);
  auto start_freq = centre_freq - bandwidth * half;
  auto start_chan_centre_freq =  start_freq + channel_bandwidth * half;

  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - bandwidth={}", bandwidth);
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - centre_freq={}", centre_freq);
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - channel_bandwidth={}", channel_bandwidth);
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - start_freq={}", start_freq);
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - start_chan_centre_freq={}", start_chan_centre_freq);

  std::vector<std::pair<double,double>> rfi_masks;
  if (data_config.has("FREQ_MASK")) {
    rfi_masks = get_rfi_masks(data_config.get_val("FREQ_MASK"));
    if (rfi_masks.size() != nmask) {
      SPDLOG_ERROR("Expected {} RFI masks but found {}", nmask, rfi_masks.size());
    }
  }

  SPDLOG_TRACE("ska::pst::stat::StatComputer::StatComputer - generating centre frequencies {} channels with channel_bandwidth={} MHz", nchan, channel_bandwidth);
  uint32_t num_masked = 0;
  for (auto ichan = 0; ichan<nchan; ichan++)
  {
    auto channel_start_freq = start_freq + static_cast<double>(ichan) * channel_bandwidth;
    auto channel_end_freq = channel_start_freq + channel_bandwidth;

    // this is mathematically the same as the average of channel_start_freq and channel_end_freq
    auto channel_centre_freq = start_chan_centre_freq + static_cast<double>(ichan) * channel_bandwidth;

    storage->channel_centre_frequencies[ichan] = channel_centre_freq;
    storage->rfi_mask_lut[ichan] = false;

    for (auto &rfi_mask_pair : rfi_masks)
    {
      // want to find if a channel freq range overlaps with the RFI mask freq range
      // There are 3 main cases:
      // 1) channel start freq < RFI mask freq but channel end freq within mask
      // 2) channel start and end freq lie within the RFI mask freq range
      // 3) channel start freq is within range but channel end freq outside of mask range
      //
      // From 2 & 3, just check if channel start freq is [RFI start, RFI end)
      // From 1, check end > RFI start and less than RFI end i.e. (RFI start, RFI end)
      if (
        (channel_start_freq >= rfi_mask_pair.first && channel_start_freq < rfi_mask_pair.second) ||
        (channel_end_freq > rfi_mask_pair.first && channel_end_freq <= rfi_mask_pair.second)
      )
      {
        SPDLOG_WARN("chan {} frequency band {:.4f} MHz to {:.4f} MHz is between {:.4f} MHz and {:.4f} MHz. Marking as masked",
          ichan, channel_start_freq, channel_end_freq,
          rfi_mask_pair.first, rfi_mask_pair.second
        );
        storage->rfi_mask_lut[ichan] = true;
        num_masked++;
        break;
      }
    }
  }

  keep_computing = true;
  initialised = true;

  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - Number of masked channels = {}", num_masked);
}

void ska::pst::stat::StatComputer::interrupt()
{
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::interrupt setting keep_computing=false");
  keep_computing = false;
}

auto ska::pst::stat::StatComputer::compute(const ska::pst::common::SegmentProducer::Segment& segment) -> bool
{
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute()");
  if (!initialised) {
    throw std::runtime_error("ska::pst::stat::StatComputer::compute - has not been initialised. StatComputer::initialised has not been called.");
  }

  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - segment.data.size={}, segment.weights.size={}", segment.data.size, segment.weights.size);

  if (segment.data.size == 0)
  {
    SPDLOG_WARN("ska::pst::stat::StatComputer::compute - segment.data.size is zero. No computation necessary");
    return false;
  }

  double utc_start_offset_seconds = static_cast<double>(segment.data.obs_offset) / bytes_per_second;
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - segment.data.obs_offset={} offset_seconds={}", segment.data.obs_offset, utc_start_offset_seconds);
  storage->set_utc_start_offset_bytes(segment.data.obs_offset);
  storage->set_utc_start_offset_seconds(utc_start_offset_seconds);

  const uint32_t nheaps = segment.data.size / heap_layout.get_data_heap_stride();
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - nheaps={}", nheaps);
  if (nheaps == 0)
  {
    std::stringstream error_msg;
    error_msg << "ska::pst::stat::StatComputer::compute - expected segment.data.size " << segment.data.size <<
      " to be at least the size of data_heap_stride " << heap_layout.get_data_heap_stride();

    SPDLOG_ERROR(error_msg.str());
    throw std::runtime_error(error_msg.str());
  }

  // warn if segment.data.size is not a multiple of data_heap_stride
  // might be at the end of the file - may not have a full heap
  if (segment.data.size % heap_layout.get_data_heap_stride() != 0)
  {
    auto data_size = nheaps * heap_layout.get_data_heap_stride();
    SPDLOG_WARN("ska::pst::stat::StatComputer::compute - effectively using only {} bytes from segment data", data_size);
  }

  // this will get the whole number of heaps to use, ignoring the partial heap
  auto expected_num_packets = nheaps * heap_layout.get_packets_per_heap();
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - expected_num_packets={}", expected_num_packets);

  // assert segment.weights.size is a multiple weights_packet_stride - do we have enough weights?
  auto expected_weights_size = expected_num_packets * heap_layout.get_weights_packet_stride();
  if (segment.weights.size != expected_weights_size) {
    SPDLOG_WARN("ska::pst::stat::StatComputer::compute - expected segment.weights.size {} to be equal to {}",
      segment.weights.size, expected_weights_size
    );
    if (segment.weights.size < expected_weights_size) {
      std::stringstream error_msg;
      error_msg << "ska::pst::stat::StatComputer::compute - expected segment.weights.size " << segment.weights.size << " to be equal to " << expected_weights_size;
      SPDLOG_ERROR(error_msg.str());
      throw std::runtime_error(error_msg.str());
    }
    SPDLOG_WARN("ska::pst::stat::StatComputer::compute - effectively using only {} bytes from segment weights", expected_weights_size);
  }

  bool all_samples_computed{false};
  // unpack the 8 or 16 bit signed integers
  if (nbit == 8) // NOLINT
  {
    all_samples_computed = compute_samples(reinterpret_cast<int8_t*>(segment.data.block), segment.weights.block, nheaps);
  }
  else if (nbit == 16) // NOLINT
  {
    all_samples_computed = compute_samples(reinterpret_cast<int16_t*>(segment.data.block), segment.weights.block, nheaps);
  }
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute all_samples_computed={}", all_samples_computed);
  return all_samples_computed;
}

template <typename T>
auto ska::pst::stat::StatComputer::compute_samples(T* data, char* weights, uint32_t nheaps) -> bool
{
  // binning_offset is used to turn convert signed data to unsiged data
  // for NBIT = 8, data goes from -128 to 127, the binning offset is 128 and bins goes from 0 to 255
  // for NBIT = 16, data goes from -32768 to 32767, the binning offset is 32768 and bins goes from 0 to 65535
  const int binning_offset = 1 << (nbit - 1);
  const int max_bin = (1 << nbit) - 1;

  // for the rebinned histograms, no changes are required for NBIT=8, but for NBIT=16, the values -128 to 127 will
  // be used. Any excess values will be clipped in the rebinned histograms, but will not result in increments
  // to the number of clipped samples.
  const int rebinning_offset = 128;
  const int max_rebin = static_cast<int>(storage->get_nrebin()) - 1;

  uint32_t packet_number{0};
  std::vector<uint32_t> pol_samples(npol,  0);
  std::vector<uint32_t> pol_samples_masked(npol,  0);

  auto total_samples_per_channel = nheaps * heap_layout.get_packet_layout().get_samples_per_packet();
  if (total_samples_per_channel % storage->get_ntime_bins() != 0)
  {
    std::stringstream error_msg;
    error_msg << "ska::pst::stat::StatComputer::compute_samples - expected ";
    error_msg << nheaps << " * nsamp_per_packet to be multiple of  " << storage->get_ntime_bins();
    throw std::runtime_error(error_msg.str());
  }
  uint32_t temporal_binning_factor = total_samples_per_channel / storage->get_ntime_bins();

  double total_sample_time = tsamp * ska::pst::common::seconds_per_microseconds * static_cast<double>(total_samples_per_channel);
  storage->set_total_sample_time(total_sample_time);

  double temporal_bin_secs = total_sample_time / static_cast<double>(storage->get_ntime_bins());

  for (auto time_bin=0; time_bin < storage->get_ntime_bins(); time_bin++)
  {
    storage->timeseries_bins[time_bin] = temporal_bin_secs * static_cast<double>(2 * time_bin + 1) / 2.0;  // NOLINT

    // need to set initial min value for timeseries to a very large value.
    for (auto i=0; i<npol; i++)
    {
      storage->timeseries[i][time_bin][TS_MIN_IDX] = std::numeric_limits<float>::max();
      storage->timeseries_masked[i][time_bin][TS_MIN_IDX] = std::numeric_limits<float>::max();
    }
  }

  if (nchan % storage->get_nfreq_bins() != 0)
  {
    std::stringstream error_msg;
    error_msg << "ska::pst::stat::StatComputer::compute_samples - expected nchan to be multiple of  " << storage->get_nfreq_bins();
    throw std::runtime_error(error_msg.str());
  }
  uint32_t freq_binning_factor = nchan / storage->get_nfreq_bins();

  // counts used in timeseries_masked mean averages
  // could initially pre-calc scale factors but easier to accumulate and use Wilford algorithm for mean.
  std::vector<std::vector<uint32_t>> timeseries_counts;
  timeseries_counts.resize(npol);
  std::vector<std::vector<uint32_t>> timeseries_counts_masked;
  timeseries_counts_masked.resize(npol);
  for (auto i=0; i<npol; i++) {
    timeseries_counts[i].resize(storage->get_ntime_bins());
    timeseries_counts_masked[i].resize(storage->get_ntime_bins());
    std::fill(timeseries_counts[i].begin(), timeseries_counts[i].end(), 0);
    std::fill(timeseries_counts_masked[i].begin(), timeseries_counts_masked[i].end(), 0);
  }

  // populate the centre freq of the frequency bins
  for (uint32_t ichan = 0; ichan<nchan; ichan += freq_binning_factor)
  {
    auto freq_bin = ichan / freq_binning_factor;
    double centre_freq{0.0};
    for (uint32_t offset = 0; offset < freq_binning_factor; offset++)
    {
      centre_freq += (storage->channel_centre_frequencies[ichan + offset]) / static_cast<double>(freq_binning_factor);
    }
    storage->frequency_bins[freq_bin] = centre_freq;
  }

  const uint32_t nsamp_per_packet = heap_layout.get_packet_layout().get_samples_per_packet();
  const uint32_t nchan_per_packet = heap_layout.get_packet_layout().get_nchan_per_packet();
  const uint32_t packets_per_heap = heap_layout.get_packets_per_heap();

  for (uint32_t iheap=0; (iheap<nheaps && keep_computing); iheap++)
  {
    const uint32_t isamp_heap = iheap * nsamp_per_packet;
    uint32_t base_ochan{0};
    for (uint32_t ipacket=0; ipacket<packets_per_heap && keep_computing; ipacket++)
    {
      const float scale_factor = get_scale_factor(weights, packet_number);

      for (uint32_t ipol=0; ipol<npol; ipol++)
      {
        for (uint32_t ichan=0; ichan<nchan_per_packet; ichan++)
        {
          const uint32_t ochan =  base_ochan + ichan;
          const bool channel_masked = storage->rfi_mask_lut[ochan];

          // linter is giving this as a false positive - clang-analyzer-core.UndefinedBinaryOperatorResult
          // nchan_per_packet and freq_binning_factor are not undefined by now.
          const uint32_t freq_bin = ochan / freq_binning_factor; // NOLINT

          for (auto isamp=0; isamp<nsamp_per_packet; isamp++)
          {
            auto osamp = isamp_heap + isamp;
            auto temporal_bin = osamp / temporal_binning_factor;
            timeseries_counts[ipol][temporal_bin] += 1;

            // needed for Wilford algorithm to calc variance
            uint32_t n_curr_channel = osamp + 1;

            pol_samples[ipol]++;
            if (!channel_masked) {
              pol_samples_masked[ipol]++;
            }

            auto value_i_int = data[I_IDX]; // NOLINT
            auto value_q_int = data[Q_IDX]; // NOLINT
            int value_i_bin = static_cast<int>(value_i_int) + binning_offset;
            int value_q_bin = static_cast<int>(value_q_int) + binning_offset;

            float value_i = static_cast<float>(value_i_int) * scale_factor;
            float value_q = static_cast<float>(value_q_int) * scale_factor;

            float power = value_i * value_i + value_q * value_q;

            // use Wilford 1962 algorithm as it is numerically stable
            auto value_i_mean_diff = value_i - storage->mean_frequency_avg[ipol][I_IDX];
            auto value_q_mean_diff = value_q - storage->mean_frequency_avg[ipol][Q_IDX];

            storage->mean_frequency_avg[ipol][I_IDX] += value_i_mean_diff/static_cast<float>(pol_samples[ipol]);
            storage->mean_frequency_avg[ipol][Q_IDX] += value_q_mean_diff/static_cast<float>(pol_samples[ipol]);

            storage->variance_frequency_avg[ipol][I_IDX] += (
              (value_i - storage->mean_frequency_avg[ipol][I_IDX]) * value_i_mean_diff
            );
            storage->variance_frequency_avg[ipol][Q_IDX] += (
              (value_q - storage->mean_frequency_avg[ipol][Q_IDX]) * value_q_mean_diff
            );

            auto value_i_mean_chan_diff = value_i - storage->mean_spectrum[ipol][I_IDX][ochan];
            auto value_q_mean_chan_diff = value_q - storage->mean_spectrum[ipol][Q_IDX][ochan];

            storage->mean_spectrum[ipol][I_IDX][ochan] += value_i_mean_chan_diff/static_cast<float>(osamp + 1);
            storage->mean_spectrum[ipol][Q_IDX][ochan] += value_q_mean_chan_diff/static_cast<float>(osamp + 1);
            storage->variance_spectrum[ipol][I_IDX][ochan] += (
              (value_i - storage->mean_spectrum[ipol][I_IDX][ochan]) * value_i_mean_chan_diff
            );
            storage->variance_spectrum[ipol][Q_IDX][ochan] += (
              (value_q - storage->mean_spectrum[ipol][Q_IDX][ochan]) * value_q_mean_chan_diff
            );

            // mean spectral power - will be normalised after unpacking as an optimisation
            storage->mean_spectral_power[ipol][ochan] += power;

            // max spectral power
            storage->max_spectral_power[ipol][ochan] = std::max(storage->max_spectral_power[ipol][ochan], power);

            // 1D histogram bins
            storage->histogram_1d_freq_avg[ipol][I_IDX][value_i_bin] += 1;
            storage->histogram_1d_freq_avg[ipol][Q_IDX][value_q_bin] += 1;

            // 1D and 2D rebinned histograms always used nrebin bins
            const int value_i_rebin = std::max(std::min(static_cast<int>(value_i_int) + rebinning_offset, max_rebin), 0);
            const int value_q_rebin = std::max(std::min(static_cast<int>(value_q_int) + rebinning_offset, max_rebin), 0);

            storage->rebinned_histogram_2d_freq_avg[ipol][value_i_rebin][value_q_rebin] += 1;
            storage->rebinned_histogram_1d_freq_avg[ipol][I_IDX][value_i_rebin] += 1;
            storage->rebinned_histogram_1d_freq_avg[ipol][Q_IDX][value_q_rebin] += 1;

            if (value_i_bin == 0 || value_i_bin == max_bin) {
              storage->num_clipped_samples_spectrum[ipol][I_IDX][ochan] += 1;
              storage->num_clipped_samples[ipol][I_IDX] += 1;
            }

            if (value_q_bin == 0 || value_q_bin == max_bin) {
              storage->num_clipped_samples_spectrum[ipol][Q_IDX][ochan] += 1;
              storage->num_clipped_samples[ipol][Q_IDX] += 1;
            }

            storage->spectrogram[ipol][freq_bin][temporal_bin] += power;
            storage->timeseries[ipol][temporal_bin][TS_MAX_IDX] = std::max(storage->timeseries[ipol][temporal_bin][TS_MAX_IDX], power);
            storage->timeseries[ipol][temporal_bin][TS_MIN_IDX] = std::min(storage->timeseries[ipol][temporal_bin][TS_MIN_IDX], power);
            storage->timeseries[ipol][temporal_bin][TS_MEAN_IDX] += (
              (power - storage->timeseries[ipol][temporal_bin][TS_MEAN_IDX]) / static_cast<float>(timeseries_counts[ipol][temporal_bin])
            );

            // handle masked channel stats
            if (!channel_masked) {
              timeseries_counts_masked[ipol][temporal_bin] += 1;

              auto value_i_mean_diff_masked = value_i - storage->mean_frequency_avg_masked[ipol][I_IDX];
              auto value_q_mean_diff_masked = value_q - storage->mean_frequency_avg_masked[ipol][Q_IDX];

              storage->mean_frequency_avg_masked[ipol][I_IDX] += value_i_mean_diff_masked/static_cast<float>(pol_samples_masked[ipol]);
              storage->mean_frequency_avg_masked[ipol][Q_IDX] += value_q_mean_diff_masked/static_cast<float>(pol_samples_masked[ipol]);
              storage->variance_frequency_avg_masked[ipol][I_IDX] += (
                (value_i - storage->mean_frequency_avg_masked[ipol][I_IDX]) * value_i_mean_diff_masked
              );
              storage->variance_frequency_avg_masked[ipol][Q_IDX] += (
                (value_q - storage->mean_frequency_avg_masked[ipol][Q_IDX]) * value_q_mean_diff_masked
              );

              // 1D histogram bins - masked
              storage->histogram_1d_freq_avg_masked[ipol][I_IDX][value_i_bin] += 1;
              storage->histogram_1d_freq_avg_masked[ipol][Q_IDX][value_q_bin] += 1;

              // 2D histogram bins - masked
              storage->rebinned_histogram_2d_freq_avg_masked[ipol][value_i_rebin][value_q_rebin] += 1;
              storage->rebinned_histogram_1d_freq_avg_masked[ipol][I_IDX][value_i_rebin] += 1;
              storage->rebinned_histogram_1d_freq_avg_masked[ipol][Q_IDX][value_q_rebin] += 1;

              // Number clipped samples - masked
              if (value_i_bin == 0 || value_i_bin == max_bin) {
                storage->num_clipped_samples_masked[ipol][I_IDX] += 1;
              }

              if (value_q_bin == 0 || value_q_bin == max_bin) {
                storage->num_clipped_samples_masked[ipol][Q_IDX] += 1;
              }

              storage->timeseries_masked[ipol][temporal_bin][TS_MAX_IDX] = std::max(storage->timeseries_masked[ipol][temporal_bin][TS_MAX_IDX], power);
              storage->timeseries_masked[ipol][temporal_bin][TS_MIN_IDX] = std::min(storage->timeseries_masked[ipol][temporal_bin][TS_MIN_IDX], power);
              storage->timeseries_masked[ipol][temporal_bin][TS_MEAN_IDX] += (
                (power - storage->timeseries_masked[ipol][temporal_bin][TS_MEAN_IDX]) / static_cast<float>(timeseries_counts_masked[ipol][temporal_bin])
              );
            }

            // update data pointer to next I & Q values
            data += 2; // NOLINT
          }
        }
      }

      base_ochan += nchan_per_packet;
      packet_number++;
    }
  }

  if (!keep_computing)
  {
    SPDLOG_WARN("Processing of statistics was interrupted and the stat storage structure is not valid");
    return false;
  }

  // technically everything after this doesn't have to be in the templated class, it can be in
  // a comman private method

  uint32_t total_sample_per_chan = nheaps * nsamp_per_packet;

  // pola and polb should have the same number of samples
  uint32_t total_samples = pol_samples[0];
  uint32_t total_samples_masked = pol_samples_masked[0];
  SPDLOG_DEBUG("total_samples={}, total_samples_masked={}, total_sample_per_chan={}",
    total_samples, total_samples_masked, total_sample_per_chan
  );

  float var_freq_factor = 1 / static_cast<float>(total_samples - 1);
  float mean_spectrum_factor = 1 / static_cast<float>(total_sample_per_chan);
  float var_freq_factor_masked = 1 / static_cast<float>(total_samples_masked - 1);
  float var_spectrum_factor = 1 / static_cast<float>(total_sample_per_chan - 1);

  for (auto ipol=0; ipol < npol; ipol++)
  {
    SPDLOG_DEBUG("storage->variance_frequency_avg[ipol][I_IDX] before norm = {}", storage->variance_frequency_avg[ipol][I_IDX]);
    storage->variance_frequency_avg[ipol][I_IDX] *= var_freq_factor;
    storage->variance_frequency_avg[ipol][Q_IDX] *= var_freq_factor;

    SPDLOG_DEBUG("ipol={}, mean_i={}, mean_q={}, var_i={}, var_q={}", ipol,
      storage->mean_frequency_avg[ipol][I_IDX], storage->mean_frequency_avg[ipol][Q_IDX],
      storage->variance_frequency_avg[ipol][I_IDX], storage->variance_frequency_avg[ipol][Q_IDX]
    );

    storage->variance_frequency_avg_masked[ipol][I_IDX] *= var_freq_factor_masked;
    storage->variance_frequency_avg_masked[ipol][Q_IDX] *= var_freq_factor_masked;

    SPDLOG_DEBUG("ipol={}, mean_i_masked={}, mean_q_masked={}, var_i_masked={}, var_q_masked={}",
      ipol,
      storage->mean_frequency_avg_masked[ipol][I_IDX], storage->mean_frequency_avg_masked[ipol][Q_IDX],
      storage->variance_frequency_avg_masked[ipol][I_IDX], storage->variance_frequency_avg_masked[ipol][Q_IDX]
    );

    for (auto ichan=0; ichan < nchan; ichan++)
    {
      storage->mean_spectral_power[ipol][ichan] *= mean_spectrum_factor;
      storage->variance_spectrum[ipol][I_IDX][ichan] *= var_spectrum_factor;
      storage->variance_spectrum[ipol][Q_IDX][ichan] *= var_spectrum_factor;
    }
  }

  return true;
}

auto ska::pst::stat::StatComputer::get_scale_factor(char * weights, uint32_t packet_number) -> float
{
  auto * weights_ptr = reinterpret_cast<float *>(weights + (packet_number * weights_packet_stride)); // NOLINT
  // return the scale factor, ignoring invalid value of 0
  if (*weights_ptr == 0) {
    return 1.0;
  } else {
    return 1 / *weights_ptr;
  }
}

auto ska::pst::stat::StatComputer::get_rfi_masks(const std::string& rfi_mask_str) -> std::vector<std::pair<double, double>>
{
  std::vector<std::pair<double,double>> masks;

  uint64_t start = 0;
  bool end = false;
  do {
    auto idx = rfi_mask_str.find(',', start);
    if (idx == std::string::npos) {
      end = true;
      idx = rfi_mask_str.length();
    }

    auto length = idx - start;
    std::string mask_pair_str = rfi_mask_str.substr(start, length);
    // +1 for delimiter
    start += (length + 1);

    // mask_pair_str should now be something like "SN:EN"
    idx = mask_pair_str.find(':', 0);
    if (idx == std::string::npos)
    {
      throw std::runtime_error("Expected RFI mask in form of S1:E1,S2:E2,...");
    }
    double start_freq = std::stod(mask_pair_str.substr(0,idx));
    double end_freq = std::stod(mask_pair_str.substr(idx+1));

    SPDLOG_INFO("ska::pst::stat::StatComputer::get_rfi_masks - masking from {:.2f} MHz to {:.2f} MHz", start_freq, end_freq);

    masks.emplace_back(std::make_pair(start_freq, end_freq));
  } while (!end);

  return masks;
}
