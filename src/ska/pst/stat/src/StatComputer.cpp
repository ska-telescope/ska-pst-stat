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
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer");

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

  // tsamp = data_config.get_double("TSAMP");
  if (data_config.has("NMASK")) {
    nmask = data_config.get_uint32("NMASK");
  }
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nmask={}", nmask);

  nsamp_per_packet = data_config.get_uint32("UDP_NSAMP");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nsamp_per_packet={}", nsamp_per_packet);

  nchan_per_packet = data_config.get_uint32("UDP_NCHAN");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nchan_per_packet={}", nchan_per_packet);

  nsamp_per_weight = data_config.get_uint32("WT_NSAMP");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - nsamp_per_weight={}", nsamp_per_weight);

  weights_packet_stride = weights_config.get_uint32("PACKET_WEIGHTS_SIZE") + weights_config.get_uint32("PACKET_SCALES_SIZE");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - weights_packet_stride={}", weights_packet_stride);

  packet_resolution = nsamp_per_packet * nchan_per_packet * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - packet_resolution={}", packet_resolution);

  heap_resolution = nsamp_per_packet * nchan * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - heap_resolution={}", heap_resolution);

  packets_per_heap = heap_resolution / packet_resolution;
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::StatComputer - packets_per_heap={}", packets_per_heap);
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
      SPDLOG_WARN("Expected {} RFI masks but found {}", nmask, rfi_masks.size());
    }
  }

  uint32_t num_masked = 0;
  for (auto ichan = 0; ichan<nchan; ichan++)
  {
    SPDLOG_TRACE("ska::pst::stat::StatComputer::StatComputer - setting centre frequency for channel {}", ichan);
    auto channel_start_freq = start_freq + static_cast<double>(ichan) * channel_bandwidth;
    auto channel_end_freq = channel_start_freq + channel_bandwidth;

    // this is mathematically the same as the average of channel_start_freq and channel_end_freq
    auto channel_centre_freq = start_chan_centre_freq + static_cast<double>(ichan) * channel_bandwidth;

    storage->channel_centre_frequencies[ichan] = static_cast<float>(channel_centre_freq);
    storage->rfi_mask_lut[ichan] = false;
    SPDLOG_TRACE("chan {} centre frequency is {:.4f} MHz, frequncy band is {:.4f} MHz to {:.4f} MHz", ichan,
      channel_centre_freq, channel_start_freq, channel_end_freq
    );

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
        (channel_end_freq > rfi_mask_pair.first && channel_end_freq < rfi_mask_pair.second)
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

  initialised = true;

  SPDLOG_DEBUG("ska::pst::stat::StatComputer::initialise() - Number of masked channels = {}", num_masked);
}

void ska::pst::stat::StatComputer::compute(
  char * data_block,
  size_t block_length,
  char * weights,
  size_t weights_length
)
{
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute()");
  if (!initialised) {
    throw std::runtime_error("ska::pst::stat::StatComputer::compute - has not been initialised. StatComputer::initialised has not been called.");
  }

  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - block_length={}, weights_length={}", block_length, weights_length);

  if (block_length == 0)
  {
    SPDLOG_WARN("ska::pst::stat::StatComputer::compute - block_length is zero. No computation necessary");
    return;
  }

  // assert block_length is a heap_resolution
  // might be at the end of the file - may not have a full heap
  if (block_length % heap_resolution != 0)
  {
    std::stringstream error_msg;
    error_msg << "ska::pst::stat::StatComputer::compute - expected block_length " << block_length <<
      " to be a multiple of heap_resolution " << heap_resolution;

    SPDLOG_WARN(error_msg.str());
    throw std::runtime_error(error_msg.str());
  }
  const uint32_t nheaps = block_length / heap_resolution;
  auto expected_num_packets = nheaps * packets_per_heap;

  // assert weights_length is a multiple weights_packet_stride - do we have enough weights?
  auto expected_weights_length = expected_num_packets * weights_packet_stride;
  if (weights_length != expected_weights_length) {
    std::stringstream error_msg;
    error_msg << "ska::pst::stat::StatComputer::compute - expected weights_length to be " << expected_weights_length;
    throw std::runtime_error(error_msg.str());
  }

  // unpack the 8 or 16 bit signed integers
  if (nbit == 8) // NOLINT
  {
    compute_samples(reinterpret_cast<int8_t*>(data_block), weights, nheaps);
  }
  else if (nbit == 16) // NOLINT
  {
    compute_samples(reinterpret_cast<int16_t*>(data_block), weights, nheaps);
  }
}

template <typename T>
void ska::pst::stat::StatComputer::compute_samples(T* data, char* weights, uint32_t nheaps)
{
  const int binning_offset = 1 << (nbit - 1);
  const int max_bin = (1 << nbit) - 1;

  uint32_t packet_number{0};
  uint32_t pola_samples{0};
  uint32_t pola_samples_masked{0};
  uint32_t polb_samples{0};
  uint32_t polb_samples_masked{0};

  auto total_samples_per_channel = nheaps * nsamp_per_packet;
  if (total_samples_per_channel % storage->get_ntime_bins() != 0)
  {
    std::stringstream error_msg;
    error_msg << "ska::pst::stat::StatComputer::compute_samples - expected ";
    error_msg << nheaps << " * nsamp_per_packet to be multiple of  " << storage->get_ntime_bins();
    throw std::runtime_error(error_msg.str());
  }
  uint32_t temporal_binning_factor = total_samples_per_channel / storage->get_ntime_bins();

  // need to set initial min value for timeseries to a very large value.
  for (auto i=0; i<npol; i++)
  {
    for (auto time_bin=0; time_bin < storage->get_ntime_bins(); time_bin++)
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
    float centre_freq{0.0};
    for (uint32_t offset = 0; offset < freq_binning_factor; offset++)
    {
      centre_freq += (storage->channel_centre_frequencies[ichan + offset]) / static_cast<float>(freq_binning_factor);
    }
  }

  for (uint32_t iheap=0; iheap<nheaps; iheap++)
  {
    for (uint32_t ipacket=0; ipacket<packets_per_heap; ipacket++)
    {
      const float scale_factor = get_weights(weights, packet_number);

      for (uint32_t ipol=0; ipol<npol; ipol++)
      {
        for (uint32_t ichan=0; ichan<nchan_per_packet; ichan++)
        {
          const uint32_t ochan = (ipacket * nchan_per_packet) + ichan;
          const bool channel_masked = storage->rfi_mask_lut[ochan];

          // linter is giving this as a false positive - clang-analyzer-core.UndefinedBinaryOperatorResult
          // nchan_per_packet and freq_binning_factor are not undefined by now.
          const uint32_t freq_bin = ochan / freq_binning_factor; // NOLINT

          for (auto isamp=0; isamp<nsamp_per_packet; isamp++)
          {
            auto osamp = iheap * nsamp_per_packet + isamp;
            auto temporal_bin = osamp / temporal_binning_factor;
            timeseries_counts[ipol][temporal_bin] += 1;

            // needed for Wilford algorithm to calc variance
            uint32_t n{0};
            uint32_t n_masked{0};
            uint32_t n_curr_channel = osamp + 1;

            if (ipol == 0) {
              pola_samples++;
              if (!channel_masked) {
                pola_samples_masked++;
              }
              n = pola_samples;
              n_masked = pola_samples_masked;
            } else {
              polb_samples++;
              if (!channel_masked) {
                polb_samples_masked++;
              }
              n = polb_samples;
              n_masked = polb_samples_masked;
            }

            auto value_i_int = data[I_IDX]; // NOLINT
            auto value_q_int = data[Q_IDX]; // NOLINT
            int value_i_bin = static_cast<int>(value_i_int) + binning_offset;
            int value_q_bin = static_cast<int>(value_q_int) + binning_offset;

            float value_i = static_cast<float>(value_i_int) / scale_factor;
            float value_q = static_cast<float>(value_q_int) / scale_factor;

            float value_i2 = powf(value_i, 2);
            float value_q2 = powf(value_q, 2);
            float power = value_i2 + value_q2;

            // use Wilford 1962 algorithm as it is numerically stable
            auto prev_mean_i = storage->mean_frequency_avg[ipol][I_IDX];
            auto prev_mean_q = storage->mean_frequency_avg[ipol][Q_IDX];

            auto value_i_mean_diff = value_i - prev_mean_i;
            auto value_q_mean_diff = value_q - prev_mean_q;

            storage->mean_frequency_avg[ipol][I_IDX] += value_i_mean_diff/static_cast<float>(n);
            storage->mean_frequency_avg[ipol][Q_IDX] += value_q_mean_diff/static_cast<float>(n);

            storage->variance_frequency_avg[ipol][I_IDX] += (
              (value_i - storage->mean_frequency_avg[ipol][I_IDX]) * value_i_mean_diff
            );
            storage->variance_frequency_avg[ipol][Q_IDX] += (
              (value_q - storage->mean_frequency_avg[ipol][Q_IDX]) * value_q_mean_diff
            );

            auto prev_chan_mean_i = storage->mean_spectrum[ipol][I_IDX][ochan];
            auto prev_chan_mean_q = storage->mean_spectrum[ipol][Q_IDX][ochan];

            auto value_i_mean_chan_diff = value_i - prev_chan_mean_i;
            auto value_q_mean_chan_diff = value_q - prev_chan_mean_q;

            storage->mean_spectrum[ipol][I_IDX][ochan] += value_i_mean_chan_diff/static_cast<float>(n_curr_channel);
            storage->mean_spectrum[ipol][Q_IDX][ochan] += value_q_mean_chan_diff/static_cast<float>(n_curr_channel);
            storage->variance_spectrum[ipol][I_IDX][ochan] += (
              (value_i - storage->mean_spectrum[ipol][I_IDX][ochan]) * value_i_mean_chan_diff
            );
            storage->variance_spectrum[ipol][Q_IDX][ochan] += (
              (value_q - storage->mean_spectrum[ipol][Q_IDX][ochan]) * value_q_mean_chan_diff
            );

            // mean spectral power
            storage->mean_spectral_power[ipol][ochan] += power/static_cast<float>(nheaps * nsamp_per_packet);

            // max spectral power
            storage->max_spectral_power[ipol][ochan] = std::max(storage->max_spectral_power[ipol][ochan], power);

            // 1D histogram bins
            storage->histogram_1d_freq_avg[ipol][I_IDX][value_i_bin] += 1;
            storage->histogram_1d_freq_avg[ipol][Q_IDX][value_q_bin] += 1;

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

              auto prev_mean_i_masked = storage->mean_frequency_avg_masked[ipol][I_IDX];
              auto prev_mean_q_masked = storage->mean_frequency_avg_masked[ipol][Q_IDX];

              auto value_i_mean_diff_masked = value_i - prev_mean_i_masked;
              auto value_q_mean_diff_masked = value_q - prev_mean_q_masked;

              storage->mean_frequency_avg_masked[ipol][I_IDX] += value_i_mean_diff_masked/static_cast<float>(n_masked);
              storage->mean_frequency_avg_masked[ipol][Q_IDX] += value_q_mean_diff_masked/static_cast<float>(n_masked);
              storage->variance_frequency_avg_masked[ipol][I_IDX] += (
                (value_i - storage->mean_frequency_avg_masked[ipol][I_IDX]) * value_i_mean_diff_masked
              );
              storage->variance_frequency_avg_masked[ipol][Q_IDX] += (
                (value_q - storage->mean_frequency_avg_masked[ipol][Q_IDX]) * value_q_mean_diff_masked
              );

              // 1D histogram bins - masked
              storage->histogram_1d_freq_avg_masked[ipol][I_IDX][value_i_bin] += 1;
              storage->histogram_1d_freq_avg_masked[ipol][Q_IDX][value_q_bin] += 1;

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
      packet_number++;
    }
  }

  // technically everything after this doesn't have to be in the templated class, it can be in
  // a comman private method

  uint32_t total_sample_per_chan = nheaps * nsamp_per_packet;
  uint32_t total_samples = pola_samples;
  uint32_t total_samples_masked = pola_samples_masked;

  SPDLOG_DEBUG("total_samples={}, total_samples_masked={}, total_sample_per_chan={}",
    total_samples, total_samples_masked, total_sample_per_chan
  );

  for (auto ipol=0; ipol < npol; ipol++)
  {
    SPDLOG_DEBUG("storage->variance_frequency_avg[ipol][I_IDX] before norm = {}", storage->variance_frequency_avg[ipol][I_IDX]);
    storage->variance_frequency_avg[ipol][I_IDX] /= static_cast<float>(total_samples - 1);
    storage->variance_frequency_avg[ipol][Q_IDX] /= static_cast<float>(total_samples - 1);

    SPDLOG_DEBUG("ipol={}, mean_i={}, mean_q={}, var_i={}, var_q={}", ipol,
      storage->mean_frequency_avg[ipol][I_IDX], storage->mean_frequency_avg[ipol][Q_IDX],
      storage->variance_frequency_avg[ipol][I_IDX], storage->variance_frequency_avg[ipol][Q_IDX]
    );

    storage->variance_frequency_avg_masked[ipol][I_IDX] /= static_cast<float>(total_samples_masked - 1);
    storage->variance_frequency_avg_masked[ipol][Q_IDX] /= static_cast<float>(total_samples_masked - 1);

    SPDLOG_DEBUG("ipol={}, mean_i_masked={}, mean_q_masked={}, var_i_masked={}, var_q_masked={}",
      ipol,
      storage->mean_frequency_avg_masked[ipol][I_IDX], storage->mean_frequency_avg_masked[ipol][Q_IDX],
      storage->variance_frequency_avg_masked[ipol][I_IDX], storage->variance_frequency_avg_masked[ipol][Q_IDX]
    );

    for (auto ichan=0; ichan < nchan; ichan++)
    {
      storage->variance_spectrum[ipol][I_IDX][ichan] /= static_cast<float>(total_sample_per_chan - 1);
      storage->variance_spectrum[ipol][Q_IDX][ichan] /= static_cast<float>(total_sample_per_chan - 1);
    }
  }

}

auto ska::pst::stat::StatComputer::get_weights(char * weights, uint32_t packet_number) -> float
{
  auto * weights_ptr = reinterpret_cast<float *>(weights + (packet_number * weights_packet_stride)); // NOLINT
  // return the scale factor, ignoring invalid value of 0
  if (*weights_ptr == 0) {
    return 1;
  } else {
    return *weights_ptr;
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
