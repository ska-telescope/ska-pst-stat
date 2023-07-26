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

#include <vector>

#include "ska/pst/common/utils/AsciiHeader.h"

#ifndef __SKA_PST_STAT_StatStorage_h
#define __SKA_PST_STAT_StatStorage_h

namespace ska::pst::stat {

  /**
   * @brief A struct that has public field for the computer update and publisher to read from.
   *
   */
  struct StatStorage
  {
    public:
      /**
       * @brief Create instance of a Stat Storage object.
       *
       * @param config the configuration current voltage data stream.
       */
      StatStorage(const ska::pst::common::AsciiHeader& config);

      /**
       * @brief Destroy the Stat Storage object.
       *
       */
      virtual ~StatStorage();

      /**
       * @brief resize the storage given the configuration
       *
       * @param ntime_bins rebinned time.
       * @param nfreq_bins rebinned frequency.
       */
      void resize(uint32_t ntime_bins, uint32_t nfreq_bins);

      /**
       * @brief reset the the storage back to zeros.
       *
       */
      void reset();

      /**
       * @brief the centre frequencies of each channel.
       */
      std::vector<float> channel_centre_frequencies;

      /**
       * @brief the mean of the data for each polarisation and dimension, averaged over all channels.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> mean_frequency_avg;

      /**
       * @brief the mean of the data for each polarisation and dimension, averaged over all channels,
       * expect those flagged for RFI.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> mean_frequency_avg_masked;

      /**
       * @brief the variance of the data for each polarisation and dimension, averaged over all channel.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> variance_frequency_avg;

      /**
       * @brief the variance of the data for each polarisation and dimension, averaged over all channels,
       * expect those flagged for RFI.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> variance_frequency_avg_masked;

      /**
       * @brief the mean of the data for each polarisation, dimension and channel.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the value for each channel.
       */
      std::vector<std::vector<std::vector<float>>> mean_spectrum;

      /**
       * @brief the variance of the data for each polarisation, dimension and channel.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the value for each channel.
       */
      std::vector<std::vector<std::vector<float>>> variance_spectrum;

      /**
       * @brief mean power spectra of the data for each polarisation and channel.
       *
       * This is real valued and computed as I^2 + Q^2.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the value for the channel.
       */
      std::vector<std::vector<float>> mean_spectral_power;

      /**
       * @brief maximum power spectra of the data for each polarisation and channel.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the value for the channel.
       */
      std::vector<std::vector<float>> max_spectral_power;

      /**
       * @brief histogram of the input data integer states for each polarisation and dimension,
       * averaged over all channels.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the binning data (i.e. the number in the bin)
       */
      std::vector<std::vector<std::vector<uint32_t>>> histogram_1d_freq_avg;

      /**
       * @brief histogram of the input data integer states for each polarisation and dimension,
       * averaged over all channels, expect those flagged for RFI.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the binning data (i.e. the number in the bin)
       */
      std::vector<std::vector<std::vector<uint32_t>>> histogram_1d_freq_avg_masked;

      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_2d_freq_avg;
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_2d_freq_avg_masked;

      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_1d_freq_avg;
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_1d_freq_avg_masked;

      /**
       * @brief number of clipped input samples (maximum level) for each polarisation, dimension and channel.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the channel number.
       */
      std::vector<std::vector<std::vector<uint32_t>>> num_clipped_samples_spectrum;

      /**
       * @brief number of clipped input samples (maximum level) for each polarisation, dimension.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<uint32_t>> num_clipped_samples;

      /**
       * @brief the timestamp offsets for each temporal bin.
       *
       * Each temporal bin used in timeseries, timeseries_masked and
       * spectrogram reuse this vector.
       */
      std::vector<float> timeseries_bins;

      /**
       * @brief the frequncy bins used for the spectrogram attribute.
       *
       */
      std::vector<float> frequnecy_bins;

      /**
       * @brief spectrogram of the data for each polarisation, averaged a
       * configurable number of temporal and spectral bins (default ~1000).
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the frequency bins, this is expected to be ~1000.
       * Third dimension is the time dimension binned.
       */
      std::vector<std::vector<std::vector<float>>> spectrogram;

      /**
       * @brief time series of the data for each polarisation, rebinned in time
       * to ntime_bins, averaged over all frequency channels.
       *
       * First dimension is polarisation (2 dimensions).
       * Second dimension is the time dimension binned.
       * Third dimension has 3 values: the max, min and mean.
       */
      std::vector<std::vector<std::vector<float>>> timeseries;

      /**
       * @brief time series of the data for each polarisation, re-binned in time
       * to ntime_bins, averaged over all frequency channels, expect those flagged by RFI.
       *
       * First dimension is polarisation (2 dimensions).
       * Second dimension is the time dimension binned.
       * Third dimension has 3 values: the max, min and mean.
       */
      std::vector<std::vector<std::vector<float>>> timeseries_masked;

    private:
      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader config;

      //! lookup table of RFI masks for each channel, true value indicates channel is masked for RFI
      std::vector<bool> rfi_mask_lut;

      //! number of temporal bins in the timeseries, timeseries_masked and spectrogram attributes
      uint32_t ntime_bins{0}; 

      //! number of spectral bins in the spectrogram
      uint32_t nfreq_bins{0}; 
  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatStorage_h
