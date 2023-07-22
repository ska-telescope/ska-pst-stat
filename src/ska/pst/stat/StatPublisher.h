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

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatStorage.h"

#ifndef __SKA_PST_STAT_StatPublisher_h
#define __SKA_PST_STAT_StatPublisher_h

namespace ska::pst::stat {

  /**
   * @brief An abstract class providing an API to publish computed statistics.
   *
   */
  class StatPublisher
  {
    public:
      /**
       * @brief Create instance of a Stat Publisher object.
       *
       * @param config the configuration current voltage data stream.
       * @param storage a shared pointer to the in memory storage of the computed statistics.
       */
      StatPublisher(const ska::pst::common::AsciiHeader& config, std::shared_ptr<StatStorage> storage);

      /**
       * @brief Destroy the Stat Publisher object.
       *
       */
      virtual ~StatPublisher();

      /**
       * @brief publish the current statistics to configured endpoint/location.
       */
      virtual void publish() = 0;

    protected:
      //! shared pointer a statistics storage, shared also with the stat process or computer
      std::shared_ptr<StatStorage> storage;

      //! the configuration for the current stream of voltage data.
      ska::pst::common::AsciiHeader config;

  };

} // ska::pst::stat

#endif