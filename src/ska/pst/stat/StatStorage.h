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
   * @brief StatStorage provides an abstract management of the storage of the
   * statistical vectors that are computed by StatComputer and published by
   * StatPublishers. The management of StatStorage is performed by the StatProcessor.
   *
   * Upon instansiation, no storage is allocated until the resize method is called.
   * As the StatComputer integrates values into the statistical vectors, the StatProcessor
   * must also use the reset method to initialise all of the vectors with value of 0.
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
       * @brief resize the storage vectors, based on the config and binning parameters
       *
       * @param ntime_bins number of temporal bins in the timeseries and spectrogram
       * @param nfreq_bins number of spectral bins in the spectrofram
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
      std::vector<double> channel_centre_frequencies;

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
       * @brief the variance of the data for each polarisation and dimension, averaged over all channels.
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

      /**
       * @brief Rebinned 2D histogram of the input data integer states for each polarisation,
       * averaged over all channels.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the bining of the real component (I)
       * Third dimension is the bining of the imaginary component (Q)
       */
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_2d_freq_avg;

      /**
       * @brief Rebinned 2D histogram of the input data integer states for each polarisation,
       * averaged over all channels, expect those flagged for RFI
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the bining of the real component (I)
       * Third dimension is the bining of the imaginary component (Q)
       */
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_2d_freq_avg_masked;

      /**
       * @brief rebinned histogram of the input data integer states for each polarisation and dimension,
       * averaged over all channels.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the binning data (i.e. the number in the bin)
       */
      std::vector<std::vector<std::vector<uint32_t>>> rebinned_histogram_1d_freq_avg;

      /**
       * @brief rebinned histogram of the input data integer states for each polarisation and dimension,
       * averaged over all channels, expect those flagged for RFI.
       *
       * First dimension is polarisation (2 dimensions)
       * Second dimension is the real and imaginary components (I and Q).
       * Third dimension is the binning data (i.e. the number in the bin)
       */
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
       * spectrogram reuse this vector. In MJD
       */
      std::vector<double> timeseries_bins;

      /**
       * @brief the frequency bins used for the spectrogram attribute (MHz).
       *
       */
      std::vector<double> frequency_bins;

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

      /**
       * @brief Lookup table of RFI masks for each channel, true value indicates channel is masked for RFI
       *
       * First dimension is channel
       */
      std::vector<bool> rfi_mask_lut;

      /**
       * @brief Get the number of polarisations used in the storage
       *
       * @return uint32_t number of polarisations
       */
      uint32_t get_npol() { return npol; };

      /**
       * @brief Get the number of dimensions used in the storage
       *
       * @return uint32_t number of dimensions
       */
      uint32_t get_ndim () { return ndim; };

      /**
       * @brief Get the number of channels used in the storage
       *
       * @return uint32_t number of channels
       */
      uint32_t get_nchan() { return nchan; };

      /**
       * @brief Get the number of input state bins used in the storage
       *
       * @return uint32_t number of input state bins
       */
      uint32_t get_nbin() { return nbin; };

      /**
       * @brief Get the number of rebinned input states used in the storage
       *
       * @return uint32_t number of rebinned input states
       */
      uint32_t get_nrebin() { return nrebin; };

      /**
       * @brief Get the number of temporal bins used in the spectrogram and timeseries storage
       *
       * @return uint32_t number of temporal bins
       */
      uint32_t get_ntime_bins() { return ntime_bins; };

      /**
       * @brief Get the number of spectral bins used in the spectrogram storage
       *
       * @return uint32_t number of spectral bins
       */
      uint32_t get_nfreq_bins() { return nfreq_bins; };

      /**
       * @brief Get the number of temporal bin dimenions in the timeseries storage [3]
       *
       * @return uint32_t number of temporal bins dimensions
       */
      uint32_t get_ntime_vals() { return ntime_vals; };

      /**
       * @brief Get the length, in seconds, of the total time used to calculate statistics.
       */
      double get_total_sample_time() { return total_sample_time; };

      /**
       * @brief Set the length, in seconds, of the total time used to calculate statistics.
       */
      void set_total_sample_time(double _total_sample_time) { total_sample_time = _total_sample_time; };

      /**
       * @brief Get flag for whether the storage has been resized
       *
       * @return true storage has been resized
       * @return false storage has not been resized
       */
      bool is_storage_resized() { return storage_resized; }

      /**
       * @brief Get flag for whether the storage has been reset
       *
       * @return true storage has been reset
       * @return false storage has not been reset
       */
      bool is_storage_reset() { return storage_reset; }

    private:

      /**
       * @brief resize the 1 dimensional vector
       *
       * @tparam T storage type of the vector
       * @param vec vector to be resized
       * @param dim1 new size of the vector
       */
      template<typename T>
      void resize_1d(std::vector<T>& vec, uint32_t dim1)
      {
        bool dim1_resized = (vec.size() != dim1);
        if (dim1_resized)
        {
          vec.resize(dim1, 0);
        }
        // if a resize has occured, then the storage is no longer in a reset state
        if (dim1_resized)
        {
          storage_reset = false;
        }
      }

      /**
       * @brief resize the 2 dimensional vector
       *
       * @tparam T storage type of the vector
       * @param vec vector to be resized
       * @param dim1 first dimension of the vector
       * @param dim2 second dimension of the vector
       */
      template<typename T>
      void resize_2d(std::vector<std::vector<T>>& vec, uint32_t dim1, uint32_t dim2)
      {
        bool dim1_resized = (vec.size() != dim1);
        if (dim1_resized)
        {
          vec.resize(dim1);
        }

        bool dim2_resized = false;
        for (uint32_t i=0; i<dim1; i++)
        {
          dim2_resized |= (vec[i].size() != dim2);
          if (dim1_resized || dim2_resized)
          {
            vec[i].resize(dim2);
          }
        }
        if (dim1_resized || dim2_resized)
        {
          storage_reset = false;
        }
      }

      /**
       * @brief resize the 3 dimensional vector
       *
       * @tparam T storage type of the vector
       * @param vec vector to be resized
       * @param dim1 first dimension of the vector
       * @param dim2 second dimension of the vector
       * @param dim3 third dimension of the vector
       */
      template<typename T>
      void resize_3d(std::vector<std::vector<std::vector<T>>>& vec, uint32_t dim1, uint32_t dim2, uint32_t dim3)
      {
        bool dim1_resized = (vec.size() != dim1);
        if (dim1_resized)
        {
          vec.resize(dim1);
        }
        bool dim2_resized = false;
        bool dim3_resized = false;
        for (uint32_t i=0; i<dim1; i++)
        {
          dim2_resized |= (vec[i].size() != dim2);
          if (dim1_resized || dim2_resized)
          {
            vec[i].resize(dim2);
          }
          for (uint32_t j=0; j<dim2; j++)
          {
            dim3_resized |= (vec[i][j].size() != dim3);
            if (dim1_resized || dim2_resized || dim3_resized)
            {
              vec[i][j].resize(dim3);
            }
          }
        }
        if (dim1_resized || dim2_resized || dim3_resized)
        {
          storage_reset = false;
        }
      }

      /**
       * @brief reset the 1 dimensional vector elements to zero
       *
       * @tparam T storage type of the vector
       * @param vec vector to reset to zero
       */
      template<typename T>
      void reset_1d(std::vector<T>& vec)
      {
        std::fill(vec.begin(), vec.end(), 0);
      }

      /**
       * @brief reset the 2 dimensional vector elements to zero
       *
       * @tparam T storage type of the vector
       * @param vec vector to reset to zero
       */
      template<typename T>
      void reset_2d(std::vector<std::vector<T>>& vec)
      {
        for (uint32_t i=0; i<vec.size(); i++)
        {
          std::fill(vec[i].begin(), vec[i].end(), 0);
        }
      }

      /**
       * @brief reset the 3 dimensional vector elements to zero
       *
       * @tparam T storage type of the vector
       * @param vec vector to reset to zero
       */
      template<typename T>
      void reset_3d(std::vector<std::vector<std::vector<T>>>& vec)
      {
        for (uint32_t i=0; i<vec.size(); i++)
        {
          for (uint32_t j=0; j<vec[i].size(); j++)
          {
            std::fill(vec[i][j].begin(), vec[i][j].end(), 0);
          }
        }
      }

      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader config;

      //! flag to indicate if the storage has been resized
      bool storage_resized{false};

      //! flag to indicate if the storage has been resized, but not reset
      bool storage_reset{false};

      //! number of temporal bins in the timeseries, timeseries_masked and spectrogram attributes
      uint32_t ntime_bins{0};

      //! number of spectral bins in the spectrogram
      uint32_t nfreq_bins{0};

      //! number of values for each timeseries bins [min, mean, max]
      uint32_t ntime_vals{3};

      //! number of polarisations represented in storage vectors
      uint32_t npol{2};

      //! number of dimensions represented in storage vectors
      uint32_t ndim{2};

      //! number of channels representated in storage vectors
      uint32_t nchan{0};

      //! number of bins represented in storage vectors
      uint32_t nbin{0};

      //! number of rebinned bins represented in storage vectors
      uint32_t nrebin{256};

      //! total time, in seconds, for sample of data the statistics are for
      double total_sample_time{0.0};
  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatStorage_h
