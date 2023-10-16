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
#include "ska/pst/stat/StatPublisher.h"

#include <spdlog/spdlog.h>
#include <memory>

#ifndef SKA_PST_STAT_ScalarStatPublisher_h
#define SKA_PST_STAT_ScalarStatPublisher_h

namespace ska::pst::stat {

  /**
   * @brief Concrete statistics publisher that only publishes scalar statistics, buffering
   * the result locally.
   *
   */
  class ScalarStatPublisher : public StatPublisher
  {
    public:
      /**
       * @brief Create instance of a ScalarStatPublisher object.
       *
       * @param config the configuration current voltage data stream.
       */
      ScalarStatPublisher(const ska::pst::common::AsciiHeader& config);

      /**
       * @brief Destroy the ScalarStatPublisher object.
       *
       */
      virtual ~ScalarStatPublisher();

      /**
       * @brief publish the computed statistics as a scalar_stats attribute.
       *
       * @param storage reference to the computed statistics to publish.
       */
      void publish(std::shared_ptr<StatStorage> storage) override;

      /**
       * @brief reset the computed statistics scalar_stats attribute.
       *
       */
      void reset();

      /**
       * @brief Return the scalar statistics of the ScalarStatPublisher.
       *
       */
      StatStorage::scalar_stats_t get_scalar_stats();

    private:
      //! mutex for protecting data access
      std::mutex scalar_stats_mutex;

      //! contains scalar statistics
      StatStorage::scalar_stats_t scalar_stats;
  };
} // namespace ska::pst::stat

#endif // SKA_PST_STAT_ScalarStatPublisher_h
