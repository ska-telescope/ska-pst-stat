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
#include "ska/pst/stat/StatPublisher.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/stat/StatComputer.h"
#include "ska/pst/common/definitions.h"

#include <memory>
#include <vector>

#ifndef __SKA_PST_STAT_StatProcessor_h
#define __SKA_PST_STAT_StatProcessor_h

namespace ska::pst::stat {

  class StatPublisher;

  /**
   * @brief A class that does the core computation of statistics per block of data.
   *
   */
  class StatProcessor
  {
    public:
      /**
       * @brief Create instance of a Stat Processor object.
       *
       * @param data_config the configuration current data stream involving the data block.
       * @param weights_config the configuration current data stream involving the weights block.
       */
      StatProcessor(const ska::pst::common::AsciiHeader& data_config, const ska::pst::common::AsciiHeader& weights_config);

      /**
       * @brief Destroy the Stat Processor object.
       *
       */
      virtual ~StatProcessor();

      /**
       * @brief
       *
       * @param publisher
       */
      void add_publisher(std::shared_ptr<StatPublisher> publisher);

      /**
       * @brief process the current block of data and weights.
       *
       * This method will ensure that the statistics are computed and then published.
       *
       * @param segment the segment of data and weights for which statistics are computed and published.
       * @return true the segment was fully processed, statistics computed and published.
       * @return false the segement was interrupted before the statistics could be computed and published.
       */
      bool process(const ska::pst::common::SegmentProducer::Segment& segment);

      /**
       * @brief Interrupt the processing of the segment, specifically the statistics computation.
       *
       */
      void interrupt();

    protected:
      //! shared pointer a statistics storage, shared also with the computer and publisher
      std::shared_ptr<StatStorage> storage;

      //! unique pointer for the stat computer.
      std::unique_ptr<StatComputer> computer;

      //! unique pointer to each of the statistics publishers.
      std::vector<std::shared_ptr<StatPublisher>> publishers;

      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader data_config;

      //! the configuration for the current stream of voltage weights.
      ska::pst::common::AsciiHeader weights_config;

      //! minimum resolution of the input data stream involving the data block
      uint32_t data_resolution;

      //! minimum resolution of the input data stream involving the weights block
      uint32_t weights_resolution;

      //! total number of channels
      uint32_t nchan{0};

      //! the requested number of time bins in the spectrogram
      uint32_t req_time_bins{0};

      //! the requested number of frequency bins in the the spectrogram
      uint32_t req_freq_bins{0};

      //! default number of time bins in spectrogram
      static constexpr uint32_t default_ntime_bins = 1024;

      //! default number of frequency bins in spectrogram
      static constexpr uint32_t default_nfreq_bins = 1024;

      //! maximum allowed number of frequency bins in spectrogram
      static constexpr uint32_t max_freq_bins = 2048;

      //! maximum allowed number of time bins in spectrogram
      static constexpr uint32_t max_time_bins = 32768;

    private:
      //! calculate correct number of bins based on block length and estimated number of bins
      auto calc_bins(uint32_t block_length, uint32_t req_bins) -> uint32_t;

  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatProcessor_h
