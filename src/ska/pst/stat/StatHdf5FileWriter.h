/*
 * Copyright 2022 Square Kilometre Array Observatory
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

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <inttypes.h>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/ValidationContext.h"
#include "ska/pst/stat/StatPublisher.h"


#ifndef SKA_PST_STAT_StatHdf5FileWriter_h
#define SKA_PST_STAT_StatHdf5FileWriter_h

namespace ska::pst::stat {

  /**
   * @brief The Stream Writer class adheres to the StateModel and provides the functionality
   * to write a data stream from a Shared Memory Ring Buffer to a series of files that are
   * stored on disk. It makes use of a DiskMonitor instance to record the data writing performance.
   *
   */
  class StatHdf5FileWriter : public ska::pst::stat::StatPublisher {

    public:

      /**
       * @brief Construct a new StatHdf5FileWriter object
       *
       */
      StatHdf5FileWriter();

      /**
       * @brief Destroy the StatHdf5FileWriter object
       *
       */
      ~StatHdf5FileWriter();

      /**
       * @brief 
       * 
       */
      void publish() override;

      /**
       * @brief Set the path object
       * 
       * @param new_path 
       */
      void set_path(std::string new_path) { stat_path = std::filesystem::path(new_path); };

    private:
      /**
       * @brief path object that indecates the location of the HDF5 file
       * 
       */
      std::filesystem::path stat_path = std::filesystem::path("/tmp");

  };

} // namespace ska::pst::stat

#endif // SKA_PST_STAT_StatHdf5FileWriter_h
