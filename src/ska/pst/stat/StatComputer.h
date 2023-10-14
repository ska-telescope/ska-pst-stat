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

#include "ska/pst/common/utils/SegmentProducer.h"
#include "ska/pst/common/utils/HeapLayout.h"
#include "ska/pst/stat/StatStorage.h"

#include <memory>
#include <utility>
#include <vector>

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
       * @param data_config the configuration current data stream involving the data block.
       * @param weights_config the configuration current data stream involving the weights block.
       * @param storage a shared pointer to the in memory storage of the computed statistics.
       */
      StatComputer(
        const ska::pst::common::AsciiHeader& data_config,
        const ska::pst::common::AsciiHeader& weights_config,
        std::shared_ptr<StatStorage> storage
      );

      /**
       * @brief Destroy the Stat Computer object.
       *
       */
      virtual ~StatComputer();

      /**
       * @brief compute the statistics for block of data.
       *
       * @param segment the segment of data and weights for which statistics are computed
       * @return true all of the input samples were processed, the stored statistics are valid
       * @return false input sampling was interrupted, the stored statistics are invalid
       */
      bool compute(const ska::pst::common::SegmentProducer::Segment& segment);

      /**
       * @brief initialise the StatComputer class in readiness for computing data.
       *
       * This method needs to be called after there has been a resize of the StatStorage, otherwise
       * there the size of data may not be as expected.
       */
      void initialise();

      /**
       * @brief interrupt the computation of statistics from the segment.
       *
       */
      void interrupt();

    private:
      //! shared pointer a statistics storage, shared between the processor and publisher
      std::shared_ptr<StatStorage> storage;

      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader data_config;

      //! the configuration for the current stream of voltage weights.
      ska::pst::common::AsciiHeader weights_config;

      //! the layout of each heap from the data and weights streams
      ska::pst::common::HeapLayout heap_layout;

      //! used to check state if the instance has been initialised
      bool initialised{false};

      //! flag that can be set to interrupt computing statistics
      bool keep_computing{true};

      template <typename T>
      bool compute_samples(T* data, char* weights, uint32_t nheaps);

      //! get scale factor for current packet
      auto get_scale_factor(char * weights, uint32_t packet_number) -> float;

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

      //! optimization: internal storage of stride avoids repeated function call in tight loop
      uint32_t weights_packet_stride{0};
  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatComputer_h
