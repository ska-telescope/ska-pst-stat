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

#include "ska/pst/stat/StatComputer.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/common/definitions.h"

static constexpr uint32_t I_INDEX = 0;
static constexpr uint32_t Q_INDEX = 1;

ska::pst::stat::StatComputer::StatComputer(
  const ska::pst::common::AsciiHeader& config,
  std::shared_ptr<StatStorage> storage
) : storage(storage) {
  // TODO - add validation
  ndim = config.get_uint32("NDIM");
  npol = config.get_uint32("NPOL");
  nbit = config.get_uint32("NBIT");
  nchan = config.get_uint32("NCHAN");
  tsamp = config.get_double("TSAMP");
  nmask = config.get_uint32("NMASK");

  nsamp_per_packet = config.get_uint32("UDP_NSAMP");
  nchan_per_packet = config.get_uint32("UDP_NCHAN");
  nsamp_per_weight = config.get_uint32("WT_NSAMP");

  weights_packet_stride = config.get_uint32("PACKET_WEIGHTS_SIZE") + config.get_uint32("PACKET_SCALES_SIZE");

  packet_resolution = nsamp_per_packet * nchan_per_packet * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  heap_resolution = nsamp_per_packet * nchan * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  packets_per_heap = heap_resolution / packet_resolution;

  // calculate the channel_centre_frequencies
  auto bandwidth = config.get_double("BW");
  auto centre_freq = config.get_double("FREQ");
  auto channel_bandwidth = bandwidth / double(nchan);
  auto start_freq = centre_freq - bandwidth / 2.0;
  auto start_chan_centre_freq =  start_freq + channel_bandwidth / 2.0;

  auto rfi_masks = get_rfi_masks(config.get_val("FREQ_MASK"));
  if (rfi_masks.size() != nmask) {
    SPDLOG_WARN("Expected {} RFI masks but found {}", nmask, rfi_masks.size());
  }

  for (auto ichan = 0; ichan<nchan; ichan++)
  {
    auto channel_start_freq = start_freq + ichan * channel_bandwidth;
    auto channel_end_freq = channel_start_freq + channel_bandwidth;

    // this is mathematically the same as the average of channel_start_freq and channel_end_freq
    auto channel_centre_freq = start_chan_centre_freq + ichan * channel_bandwidth;

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
        (channel_end_freq = rfi_mask_pair.first && channel_end_freq < rfi_mask_pair.second)
      )
      {
        storage->rfi_mask_lut[ichan] = true;
        break;
      }
    }
  }

}

void ska::pst::stat::StatComputer::compute(
  char * data_block,
  size_t block_length,
  char * weights,
  size_t weights_length
)
{
  // assert block_length is a heap_resolution
  // might be at the end of the file - may not have a full heap, what should we do?
  if (block_length % heap_resolution != 0)
  {
    throw std::runtime_error("Foobar");
  }
  const uint32_t nheaps = block_length / heap_resolution;
  auto expected_num_packets = nheaps * packets_per_heap;

  // assert weights_length is a multiple weights_packet_stride - do we have enough weights?
  auto expected_weights_length = expected_num_packets * weights_packet_stride;
  if (weights_length != expected_weights_length) {
    throw std::runtime_error("batman!");
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

  uint32_t packet_number = 0;
  uint32_t count = 0;
  uint32_t count_masked = 0;

  for (auto iheap=0; iheap<nheaps; iheap++)
  {
    for (auto ipacket=0; ipacket<packets_per_heap; ipacket++)
    {
      const float scale_factor = get_weights(weights, packet_number);

      for (auto ipol=0; ipol<npol; ipol++)
      {
        for (auto ichan=0; ichan<nchan_per_packet; ichan++)
        {
          const auto ochan = (ipacket * nchan_per_packet) + ichan;
          for (auto isamp=0; isamp<nsamp_per_packet; isamp++)
          {
            auto value_i_int = data[I_INDEX]; // data[0]
            auto value_q_int = data[Q_INDEX]; // data[1]
            int value_i_bin = static_cast<int>(value_i_int) + binning_offset;
            int value_q_bin = static_cast<int>(value_q_int) + binning_offset;

            auto value_i = value_i_int / scale_factor;
            auto value_q = value_q_int / scale_factor;

            auto value_i2 = value_i * value_i;
            auto value_q2 = value_q * value_q;
            auto power = value_i2 + value_q2;

            // accumulate value across all channels - will be normalised later
            storage->mean_frequency_avg[ipol][I_INDEX] += value_i;
            storage->mean_frequency_avg[ipol][Q_INDEX] += value_q;

            // accumulate value for channel - will be normalised later
            storage->mean_spectrum[ipol][I_INDEX][ochan] += value_i;
            storage->mean_spectrum[ipol][Q_INDEX][ochan] += value_q;

            // accumulate value2 across all channels - will be converted to var later
            storage->variance_frequency_avg[ipol][I_INDEX] += value_i2;
            storage->variance_frequency_avg[ipol][Q_INDEX] += value_q2;

            // accumulate value2 for channels - will be converted to var later
            storage->variance_spectrum[ipol][I_INDEX][ochan] += value_i2;
            storage->variance_spectrum[ipol][Q_INDEX][ochan] += value_q2;

            count += 1;
            if (!storage->rfi_mask_lut[ichan]) {
              storage->mean_frequency_avg_masked[ipol][I_INDEX] += value_i;
              storage->mean_frequency_avg_masked[ipol][Q_INDEX] += value_q;
              storage->variance_frequency_avg_masked[ipol][I_INDEX] += value_i2;
              storage->variance_frequency_avg_masked[ipol][Q_INDEX] += value_q2;
              count_masked += 1;
            }

            // accumulate power - will be normalised later
            storage->mean_spectral_power[ipol][ochan] += power;

            // max power
            storage->mean_spectral_power[ipol][ochan] = std::max(storage->mean_spectral_power[ipol][ochan], power);

            // 1D histogram bins
            storage->histogram_1d_freq_avg[ipol][I_INDEX][value_i_bin] += 1;
            storage->histogram_1d_freq_avg[ipol][Q_INDEX][value_q_bin] += 1;

            if (value_i_bin == 0 || value_i_bin == max_bin) {
              storage->num_clipped_samples_spectrum[ipol][I_INDEX][ochan] += 1;
              storage->num_clipped_samples[ipol][I_INDEX] += 1;
            }

            if (value_q_bin == 0 || value_q_bin == max_bin) {
              storage->num_clipped_samples_spectrum[ipol][Q_INDEX][ochan] += 1;
              storage->num_clipped_samples[ipol][Q_INDEX] += 1;
            }

            // update data pointer to next I & Q values
            data += 2;
          }
        }
      }
      packet_number++;
    }
  }

  // technically everything after this doesn't have to be in the templated class, it can be in
  // a comman private method

  // our counts include both polarisations, need to divide npol. This is used for calculating averages
  count /= npol;
  count_masked /= npol;
  auto count_per_channel = count / nchan;

  for (auto ipol=0; ipol < npol; ipol++)
  {
    auto mean_i = storage->mean_frequency_avg[ipol][I_INDEX] / count;
    storage->mean_frequency_avg[ipol][I_INDEX] = mean_i;

    auto mean_q = storage->mean_frequency_avg[ipol][Q_INDEX] / count;
    storage->mean_frequency_avg[ipol][Q_INDEX] = mean_q;

    // Var(X) = E(X^2) - E(X)^2
    // we have previously stored the Sum(X^2) in each dimension, divide by count
    // E(X) is just the mean
    auto var_i = storage->variance_frequency_avg[ipol][I_INDEX] / count - mean_i * mean_i;
    storage->variance_frequency_avg[ipol][I_INDEX] = var_i;

    auto var_q = storage->variance_frequency_avg[ipol][Q_INDEX] / count - mean_q * mean_q;
    storage->variance_frequency_avg[ipol][Q_INDEX] = var_q;

    auto mean_i_masked = storage->mean_frequency_avg_masked[ipol][I_INDEX] / count_masked;
    storage->mean_frequency_avg_masked[ipol][I_INDEX] = mean_i_masked;

    auto mean_q_masked = storage->mean_frequency_avg_masked[ipol][Q_INDEX] / count_masked;
    storage->mean_frequency_avg_masked[ipol][Q_INDEX] = mean_q_masked;

    auto var_i_masked = storage->variance_frequency_avg_masked[ipol][I_INDEX] / count_masked - mean_i_masked * mean_i_masked;
    storage->variance_frequency_avg_masked[ipol][I_INDEX] = var_i_masked;

    auto var_q_masked = storage->variance_frequency_avg_masked[ipol][Q_INDEX] / count_masked - mean_q_masked * mean_q_masked;
    storage->variance_frequency_avg_masked[ipol][Q_INDEX] = var_q_masked;

    for (auto ichan=0; ichan < nchan; ichan++)
    {
      auto chan_mean_i = storage->mean_spectrum[ipol][I_INDEX][ichan] / count_per_channel;
      storage->mean_frequency_avg[ipol][I_INDEX] = chan_mean_i;

      auto chan_mean_q = storage->mean_spectrum[ipol][Q_INDEX][ichan] / count_per_channel;
      storage->mean_frequency_avg[ipol][Q_INDEX] = chan_mean_q;

      auto chan_var_i = storage->variance_spectrum[ipol][I_INDEX][ichan] / count_per_channel - chan_mean_i * chan_mean_i;
      storage->variance_spectrum[ipol][I_INDEX][ichan] = chan_var_i;

      auto chan_var_q = storage->variance_spectrum[ipol][Q_INDEX][ichan] / count_per_channel - chan_mean_q * chan_mean_q;
      storage->variance_spectrum[ipol][Q_INDEX][ichan] = chan_var_q;

      storage->mean_spectral_power[ipol][ichan] /= count_per_channel;
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

auto ska::pst::stat::StatComputer::get_rfi_masks(std::string rfi_mask_str) -> std::vector<std::pair<double, double>>
{
  std::vector<std::pair<double,double>> masks;

  int start = 0;
  int mask_idx = 0;
  do {
    int idx = rfi_mask_str.find(',', start);
    if (idx == std::string::npos) {
      break;
    }

    int length = idx - start;
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

    masks.push_back(std::make_pair(start_freq, end_freq));
  } while (true);

  return masks;
}
