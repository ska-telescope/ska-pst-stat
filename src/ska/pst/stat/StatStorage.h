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

#include "ska/pst/common/utils/AsciiHeader.h"
#include <vector>

#ifndef __SKA_PST_STAT_StatStorage_h
#define __SKA_PST_STAT_StatStorage_h

namespace ska {
namespace pst {
namespace stat {

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
      StatStorage(ska::pst::common::AsciiHeader config);

      /**
       * @brief Destroy the Stat Storage object.
       *
       */
      virtual ~StatStorage();

      /**
       * @brief resize the storage given the configuration
       *
       * @param config new configuration to apply to determine size of storage.
       */
      void resize(ska::pst::common::AsciiHeader config);

      /**
       * @brief reset the the storage back to zeros.
       *
       */
      void reset();

      /**
       * @brief the average mean frequency of the data across all channels.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> mean_frequency_avg;

      /**
       * @brief the average mean frequency of the data across all channels with RFI masking applied.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> mean_frequency_avg_masked;

      /**
       * @brief the average variance of the frequency of the data across all channels.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> variance_frequency_avg;

      /**
       * @brief the average variance of the frequency of the data across all channels with RFI masking applied.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<float>> variance_frequency_avg_masked;

      /**
       * @brief a spectrum of mean frequency of the data.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the value for each channel.
       */
      std::vector<std::vector<std::vector<float>>> mean_spectrum;

      /**
       * @brief a spectrum of variancy frequency of the data.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the value for each channel.
       */
      std::vector<std::vector<std::vector<float>>> variance_spectrum;

      /**
       * @brief the bandpass of the data.
       *
       * This is real valued as is a power spectrum
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the value for the channel.
       */
      std::vector<std::vector<float>> bandpass;

      /**
       * @brief a 1D histrogram of frequency.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the binning data (i.e. the number in the bin)
       */
      std::vector<std::vector<std::vector<uint32_t>>> histogram_1d_freq_avg;

      /**
       * @brief a 1D histrogram of frequency with RFI masking applied.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the binning data (i.e. the number in the bin)
       */
      std::vector<std::vector<std::vector<uint32_t>>> histogram_1d_freq_avg_masked;

      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_2d_freq_avg;
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_2d_freq_avg_masked;

      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_1d_freq_avg;
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_1d_freq_avg_masked;

      /**
       * @brief the number of clipped samples per polarisation and dimension, per channel.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the channel number.
       */
      std::vector<std::vector<std::vector<uint32_t>>> num_clipped_samples_spectrum;

      /**
       * @brief the number of clipped samples per polarisation and dimension.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the real and imaginary components (I and Q).
       */
      std::vector<std::vector<uint32_t>> num_clipped_samples;

      /**
       * @brief a spectrogram of the data.
       *
       * First dimension is polarisation (2 dimenions)
       * Second dimension is the frequency bins, this is expecte to be ~1000.
       * Third dimension is the time dimension binned.
       */
      std::vector<std::vector<std::vector<float>>> spectrogram;

      /**
       * @brief this is a time series of the data.
       *
       * First dimension is polarisation (2 dimenions).
       * Second dimension is the time dimension binned.
       * Third dimension has 3 values: the max, min, and mean.
       */
      std::vector<std::vector<std::vector<float>>> timeseries;

      /**
       * @brief this is a time series of the data with RFI masking applied.
       *
       * First dimension is polarisation (2 dimenions).
       * Second dimension is the time dimension binned.
       * Third dimension has 3 values: the max, min, and mean.
       */
      std::vector<std::vector<std::vector<float>>> timeseries_masked;

    private:
      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader config;

  }

} // stat
} // pst
} // ska
#endif __SKA_PST_STAT_StatStorage_h
