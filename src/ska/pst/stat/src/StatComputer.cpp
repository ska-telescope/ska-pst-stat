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

#include "ska/pst/stat/StatComputer.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/common/definitions.h"

static constexpr uint32_t I_INDEX = 0;
static constexpr uint32_t Q_INDEX = 1;
static constexpr float half = 1.0/2.0;

ska::pst::stat::StatComputer::StatComputer(
  const ska::pst::common::AsciiHeader& config,
  const std::shared_ptr<StatStorage>&  storage
) : storage(storage) {
  // TODO - add validation
  ndim = config.get_uint32("NDIM");
  npol = config.get_uint32("NPOL");
  nbit = config.get_uint32("NBIT");
  nchan = config.get_uint32("NCHAN");
  // tsamp = config.get_double("TSAMP");
  if (config.has("NMASK")) {
    nmask = config.get_uint32("NMASK");
  }

  nsamp_per_packet = config.get_uint32("UDP_NSAMP");
  nchan_per_packet = config.get_uint32("UDP_NCHAN");
  nsamp_per_weight = config.get_uint32("WT_NSAMP");

  weights_packet_stride = config.get_uint32("PACKET_WEIGHTS_SIZE") + config.get_uint32("PACKET_SCALES_SIZE");

  // calculate the channel_centre_frequencies
  auto bandwidth = config.get_double("BW");
  auto centre_freq = config.get_double("FREQ");
  auto channel_bandwidth = bandwidth / static_cast<double>(nchan);
  auto start_freq = centre_freq - bandwidth * half;
  auto start_chan_centre_freq =  start_freq + channel_bandwidth * half;

  std::vector<std::pair<double,double>> rfi_masks;
  if (config.has("FREQ_MASK")) {
    rfi_masks = get_rfi_masks(config.get_val("FREQ_MASK"));
    if (rfi_masks.size() != nmask) {
      SPDLOG_WARN("Expected {} RFI masks but found {}", nmask, rfi_masks.size());
    }
  }

  uint32_t num_masked = 0;
  for (auto ichan = 0; ichan<nchan; ichan++)
  {
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

  packet_resolution = nsamp_per_packet * nchan_per_packet * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  heap_resolution = nsamp_per_packet * nchan * npol * ndim * nbit / ska::pst::common::bits_per_byte;
  packets_per_heap = heap_resolution / packet_resolution;

  SPDLOG_DEBUG("Number of masked channels = {}", num_masked);
}

void ska::pst::stat::StatComputer::compute(
  char * data_block,
  size_t block_length,
  char * weights,
  size_t weights_length
)
{
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - start");
  SPDLOG_DEBUG("ska::pst::stat::StatComputer::compute - block_length={}, weights_length={}", block_length, weights_length);

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
          const bool channel_masked = storage->rfi_mask_lut[ochan];
          for (auto isamp=0; isamp<nsamp_per_packet; isamp++)
          {
            // needed for Wilford algorithm to calc variance
            uint32_t n{0};
            uint32_t n_masked{0};
            uint32_t n_curr_channel = iheap * nsamp_per_packet + isamp + 1;

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

            auto value_i_int = data[I_INDEX]; // NOLINT
            auto value_q_int = data[Q_INDEX]; // NOLINT
            int value_i_bin = static_cast<int>(value_i_int) + binning_offset;
            int value_q_bin = static_cast<int>(value_q_int) + binning_offset;

            float value_i = static_cast<float>(value_i_int) / scale_factor;
            float value_q = static_cast<float>(value_q_int) / scale_factor;

            float value_i2 = powf(value_i, 2);
            float value_q2 = powf(value_q, 2);
            float power = value_i2 + value_q2;

            // use Wilford 1962 algorithm as it is numerically stable
            auto prev_mean_i = storage->mean_frequency_avg[ipol][I_INDEX];
            auto prev_mean_q = storage->mean_frequency_avg[ipol][Q_INDEX];

            auto value_i_mean_diff = value_i - prev_mean_i;
            auto value_q_mean_diff = value_q - prev_mean_q;

            storage->mean_frequency_avg[ipol][I_INDEX] += value_i_mean_diff/static_cast<float>(n);
            storage->mean_frequency_avg[ipol][Q_INDEX] += value_q_mean_diff/static_cast<float>(n);

            storage->variance_frequency_avg[ipol][I_INDEX] += (
              (value_i - storage->mean_frequency_avg[ipol][I_INDEX]) * value_i_mean_diff
            );
            storage->variance_frequency_avg[ipol][Q_INDEX] += (
              (value_q - storage->mean_frequency_avg[ipol][Q_INDEX]) * value_q_mean_diff
            );

            auto prev_chan_mean_i = storage->mean_spectrum[ipol][I_INDEX][ochan];
            auto prev_chan_mean_q = storage->mean_spectrum[ipol][Q_INDEX][ochan];

            auto value_i_mean_chan_diff = value_i - prev_chan_mean_i;
            auto value_q_mean_chan_diff = value_q - prev_chan_mean_q;

            storage->mean_spectrum[ipol][I_INDEX][ochan] += value_i_mean_chan_diff/static_cast<float>(n_curr_channel);
            storage->mean_spectrum[ipol][Q_INDEX][ochan] += value_q_mean_chan_diff/static_cast<float>(n_curr_channel);
            storage->variance_spectrum[ipol][I_INDEX][ochan] += (
              (value_i - storage->mean_spectrum[ipol][I_INDEX][ochan]) * value_i_mean_chan_diff
            );
            storage->variance_spectrum[ipol][Q_INDEX][ochan] += (
              (value_q - storage->mean_spectrum[ipol][Q_INDEX][ochan]) * value_q_mean_chan_diff
            );

            if (!channel_masked) {
              auto prev_mean_i_masked = storage->mean_frequency_avg_masked[ipol][I_INDEX];
              auto prev_mean_q_masked = storage->mean_frequency_avg_masked[ipol][Q_INDEX];

              auto value_i_mean_diff_masked = value_i - prev_mean_i_masked;
              auto value_q_mean_diff_masked = value_q - prev_mean_q_masked;

              storage->mean_frequency_avg_masked[ipol][I_INDEX] += value_i_mean_diff_masked/static_cast<float>(n_masked);
              storage->mean_frequency_avg_masked[ipol][Q_INDEX] += value_q_mean_diff_masked/static_cast<float>(n_masked);
              storage->variance_frequency_avg_masked[ipol][I_INDEX] += (
                (value_i - storage->mean_frequency_avg_masked[ipol][I_INDEX]) * value_i_mean_diff_masked
              );
              storage->variance_frequency_avg_masked[ipol][Q_INDEX] += (
                (value_q - storage->mean_frequency_avg_masked[ipol][Q_INDEX]) * value_q_mean_diff_masked
              );
            }

            // mean spectral power
            storage->mean_spectral_power[ipol][ochan] += power/static_cast<float>(nheaps * nsamp_per_packet);

            // max spectral power
            storage->max_spectral_power[ipol][ochan] = std::max(storage->max_spectral_power[ipol][ochan], power);

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
    SPDLOG_DEBUG("storage->variance_frequency_avg[ipol][I_INDEX] before norm = {}", storage->variance_frequency_avg[ipol][I_INDEX]);
    storage->variance_frequency_avg[ipol][I_INDEX] /= static_cast<float>(total_samples - 1);
    storage->variance_frequency_avg[ipol][Q_INDEX] /= static_cast<float>(total_samples - 1);

    SPDLOG_DEBUG("ipol={}, mean_i={}, mean_q={}, var_i={}, var_q={}", ipol,
      storage->mean_frequency_avg[ipol][I_INDEX], storage->mean_frequency_avg[ipol][Q_INDEX],
      storage->variance_frequency_avg[ipol][I_INDEX], storage->variance_frequency_avg[ipol][Q_INDEX]
    );

    storage->variance_frequency_avg_masked[ipol][I_INDEX] /= static_cast<float>(total_samples_masked - 1);
    storage->variance_frequency_avg_masked[ipol][Q_INDEX] /= static_cast<float>(total_samples_masked - 1);

    SPDLOG_DEBUG("ipol={}, mean_i_masked={}, mean_q_masked={}, var_i_masked={}, var_q_masked={}",
      ipol,
      storage->mean_frequency_avg_masked[ipol][I_INDEX], storage->mean_frequency_avg_masked[ipol][Q_INDEX],
      storage->variance_frequency_avg_masked[ipol][I_INDEX], storage->variance_frequency_avg_masked[ipol][Q_INDEX]
    );

    for (auto ichan=0; ichan < nchan; ichan++)
    {
      storage->variance_spectrum[ipol][I_INDEX][ichan] /= static_cast<float>(total_sample_per_chan - 1);
      storage->variance_spectrum[ipol][Q_INDEX][ichan] /= static_cast<float>(total_sample_per_chan - 1);
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
