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
#include <string>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatPublisher.h"

#ifndef __SKA_PST_STAT_StatHdf5FileWriter_h
#define __SKA_PST_STAT_StatHdf5FileWriter_h

namespace ska::pst::stat {

  /**
   * @brief An implementation of the StatPublisher that dumps the data to a HDF5 file.
   *
   */
  class StatHdf5FileWriter : public StatPublisher
  {
    public:
      /**
       * @brief Create instance of a Stat HDF5 File Writer object.
       *
       * @param config the configuration current voltage data stream.
       * @param storage a shared pointer to the in memory storage of the computed statistics.
       * @param file_path path of where to write data out to.
       */
      StatHdf5FileWriter(
        const ska::pst::common::AsciiHeader& config,
        std::shared_ptr<StatStorage> storage,
        const std::string& file_path
      );

      /**
       * @brief Destroy the Stat HDF5 File Writer object.
       *
       */
      virtual ~StatHdf5FileWriter();

      /**
       * @brief publish the computed statistics as a HDF5 file.
       */
      void publish() override;

  };

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatHdf5FileWriter_h
