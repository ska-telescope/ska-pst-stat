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
#include "ska/pst/common/statemodel/ApplicationManager.h"
#include <string>
#include <memory>

#ifndef __SKA_PST_STAT_StatApplicationManager_h
#define __SKA_PST_STAT_StatApplicationManager_h

namespace ska::pst::stat {

  /**
   * @brief Manager for running SKA PST STAT within a pipeline.
   *
   */
  class StatApplicationManager : public ska::pst::common::ApplicationManager
  {
    public:
      /**
       * @brief Create instance of a Stat Application Manager object.
       *
       */
      StatApplicationManager();

      /**
       * @brief Destroy the Stat Application Manager object.
       *
       */
      virtual ~StatApplicationManager();

      /**
       * @brief Initialisation callback.
       *
       */
      void perform_initialise();

      /**
       * @brief Beam Configuration callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from Idle to BeamConfigured.
       *
       */
      void perform_configure_beam();

      /**
       * @brief Scan Configuration callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from BeamConfigured to ScanConfigured.
       *
       */
      void perform_configure_scan();

      /**
       * @brief StartScan callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from ScanConfigured to Scanning.
       * Launches perform_scan on a separate thread.
       *
       */
      void perform_start_scan();

      /**
       * @brief Scan callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains scanning instructions meant to be launched in a separate thread.
       *
       */
      void perform_scan();

      /**
       * @brief StopScan callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from Scanning to ScanConfigured.
       *
       */
      void perform_stop_scan();

      /**
       * @brief DeconfigureScan callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from ScanConfigured to BeamConfigured.
       *
       */
      void perform_deconfigure_scan();

      /**
       * @brief DeconfigureBeam callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from BeamConfigured to Idle.
       *
       */
      void perform_deconfigure_beam();

      /**
       * @brief Terminate callback that is called by \ref ska::pst::common::ApplicationManager::main.
       * Contains the instructions required prior to transitioning the state from RuntimeError to Idle.
       *
       */
      void perform_terminate();

    private:
      //! shared pointer a statistics processor
      std::shared_ptr<StatProcessor> processor;

      //! shared pointer to a view of the data ring buffer
      std::shared_ptr<DataBlockStats> data_rb_view;

      //! shared pointer to a view of the weights ring buffer
      std::shared_ptr<DataBlockStats> weights_rb_view;
  }

} // namespace ska::pst::stat

#endif // __SKA_PST_STAT_StatApplicationManager_h
