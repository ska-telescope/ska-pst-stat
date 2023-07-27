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

#include "ska/pst/common/definitions.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/stat/StatComputer.h"
#include "ska/pst/stat/StatHdf5FileWriter.h"

#ifndef __SKA_PST_STAT_StatProcessor_h
#define __SKA_PST_STAT_StatProcessor_h

namespace ska::pst::stat {

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
       * @param config the configuration current voltage data stream.
       */
      StatProcessor(const ska::pst::common::AsciiHeader& config);

      /**
       * @brief Destroy the Stat Processor object.
       *
       */
      virtual ~StatProcessor();

      /**
       * @brief process the current block of data and weights.
       *
       * This method will ensure that the statistics are computed and then published.
       *
       * @param data_block a pointer to the start of the data block to compute statistics for.
       * @param block_length the size, in bytes, of the data block to process.
       * @param weights_block a pointer to the start of the weights block to use in computing statistics.
       * @param weights_length the size, in bytes, of the weights to process.
       */
      void process(char * data_block, size_t block_length, char * weights_block, size_t weights_length);

    protected:
      //! shared pointer a statistics storage, shared also with the computer and publisher
      std::shared_ptr<StatStorage> storage;

      //! unique pointer for the stat computer.
      std::unique_ptr<StatComputer> computer;

      //! unique pointer for the stat publisher.
      std::unique_ptr<StatPublisher> publisher;

      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader config;

      //! minimum resolution of the input data stream
      uint32_t data_resolution;

      uint32_t req_time_bins{1024};

      uint32_t req_freq_bins{1024};

      uint32_t max_freq_bins{2048};

      uint32_t max_time_bins{32786};

  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatProcessor_h
