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

#include <memory>
#include <utility>
#include <vector>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatStorage.h"

#ifndef __SKA_PST_STAT_StatComputer_h
#define __SKA_PST_STAT_StatComputer_h

namespace ska::pst::stat {

  /**
   * @brief Class used for processing a stream of voltage data, including weights.
   *
   */
  class StatComputer
  {
    public:
      /**
       * @brief Create instance of a Stat Computer object.
       *
       * @param config the configuration current voltage data stream.
       * @param storage a shared pointer to the in memory storage of the computed statistics.
       */
      StatComputer(const ska::pst::common::AsciiHeader& config, const std::shared_ptr<StatStorage>& storage);

      /**
       * @brief Destroy the Stat Computer object.
       *
       */
      virtual ~StatComputer() = default;

      /**
       * @brief compute the statistics for block of data.
       *
       * @param data_block a pointer to the start of the data block to compute statistics for.
       * @param block_length the size, in bytes, of the data block to process.
       * @param weights a pointer to the start of the weights block to use in computing statistics.
       * @param weights_length the size, in bytes, of the weights to process.
       */
      void compute(char * data_block, size_t block_length, char * weights, size_t weights_length);

    private:
      //! shared pointer a statistics storage, shared between the processor and publisher
      std::shared_ptr<StatStorage> storage;

      template <typename T>
      void compute_samples(T* data, char* weights, uint32_t nheaps);

      //! get weights for current packet
      auto get_weights(char * weights, uint32_t packet_number) -> float;

      //! get RFI masks
      auto get_rfi_masks(const std::string& rfi_mask) -> std::vector<std::pair<double, double>>;

      //! Time per sample (in microsecs), used for populating time timeseries_bins
      double tsamp{0.0};

      //! Number of polarisations in the data stream
      uint32_t npol{0};

      //! Number of dimensions in the data stream
      uint32_t ndim{0};

      //! Number of channels in the data stream
      uint32_t nchan{0};

      //! Number of bits per sample in the data stream
      uint32_t nbit{0};

      //! Number of RFI channels that will be masked
      uint32_t nmask{0};

      //! Number of bits per sample in the weights stream
      uint32_t weights_nbit{0};

      //! Number of samples per UDP packet in the data stream
      uint32_t nsamp_per_packet{0};

      //! Number of channels per UDP packet in the data stream
      uint32_t nchan_per_packet{0};

      //! Number of samples per relative weight in the weights stream
      uint32_t nsamp_per_weight{0};

      //! Number of bytes per packet in the weights stream
      uint32_t weights_packet_stride{0};

      //! Size of a complete heap of data in the data stream, in btyes
      uint32_t heap_resolution{0};

      //! Size of the complex packet of data in the data stream, in bytes
      uint32_t packet_resolution{0};

      //! Number of UDP packets per heap in the data stream
      uint32_t packets_per_heap{0};

  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatComputer_h
