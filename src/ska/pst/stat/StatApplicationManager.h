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
#include "ska/pst/smrb/SmrbSegmentProducer.h"
#include "ska/pst/stat/ScalarStatPublisher.h"
#include "ska/pst/stat/StatHdf5FileWriter.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/stat/StatProcessor.h"

#include <string>
#include <memory>
#include <vector>
#include <mutex>

#ifndef SKA_PST_STAT_StatApplicationManager_h
#define SKA_PST_STAT_StatApplicationManager_h

namespace ska::pst::stat {

  /**
   * @brief Manager for running SKA PST STAT within a pipeline.
   *
   */
  class StatApplicationManager : public ska::pst::common::ApplicationManager
  {
    public:

      /**
       * @brief Enumeration of statistics processor's state
       *
       */
      enum ProcessingState {
        Unknown,
        Idle,
        Processing,
        Waiting,
      };

      /**
       * @brief Create instance of a Stat Application Manager object.
       *
       * @param base_path directory to which stat hdf5 files will be written.
       */
      StatApplicationManager(std::string base_path);

      /**
       * @brief Destroy the Stat Application Manager object.
       *
       */
      ~StatApplicationManager();

      /**
       * @brief Get the base path the StatApplicationManager is constructed with.
       *
       * @return std::string filesystem base path to where files will be written
       */
      std::string get_stat_base_path() { return stat_base_path; };

      /**
       * @brief Configure beam and scan as described by the configuration file
       *
       * @param config_file configuration file containing beam and scan configuration parameters
       */
      void configure_from_file(const std::string &config_file);

      /**
       * @brief Configure beam as described by the configuration parameters
       *
       * @param config beam configuration containing the recording path, data and weights key
       * @param context A validation context where errors should be added.
       */
      void validate_configure_beam(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context);

      /**
       * @brief Configure scan as described by the configuration parameters
       *
       * @param config scan configuration containing the bytes_per_second and scan_len_max
       * @param context A validation context where errors should be added.
       */
      void validate_configure_scan(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context);

      /**
       * @brief Start scan as described by the configuration parameters
       *
       * @param config start scan configuration
       */
      void validate_start_scan(const ska::pst::common::AsciiHeader& config);

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

      /**
       * @brief Set the timeout for connecting to the DataBlockView object.
       *
       */
      void set_timeout(int _timeout) { timeout = _timeout; }

      /**
       * @brief Get the scalar stats object
       *
       * @return StatStorage::scalar_stats_t
       */
      StatStorage::scalar_stats_t get_scalar_stats();

      /**
       * @brief Get the processing state of the application manager
       *
       * @return State processing state of the application manager
       */
      ProcessingState get_processing_state() { return processing_state; };

    private:

      //! Current processing state of the application manager
      ProcessingState processing_state{Unknown};

      //! directory to which stat HDF5 files will be written.
      std::string stat_base_path;

      //! timeout, in seconds, to wait when attempting to connect to the DataBlockView object.
      int timeout{120};

      //! delay, in milliseconds, to wait between processing data from the producer
      uint32_t processing_delay{5000};

      //! flag to indicate if processing of statistics should be contining
      bool keep_processing{true};

      //! hexidecimal shared memory key for the data ring buffer
      std::string data_key;

      //! hexidecimal shared memory key for the weights ring buffer
      std::string weights_key;

      //! requested number of time bins for statistics
      uint32_t req_time_bins{0};

      //! requested number of freq bins for statistics
      uint32_t req_freq_bins{0};

      //! number of bins to re-bin the large histograms into
      uint32_t num_rebin{0};

      //! Coordinates thread interactions on the keep_processing attribute
      std::condition_variable processing_cond;

      //! Protect the keep_processing condition variable
      std::mutex processing_mutex;

      //! received headers from the segment of data and weights
      ska::pst::common::AsciiHeader data_header;
      ska::pst::common::AsciiHeader weights_header;

      //! shared pointer a statistics processor
      std::shared_ptr<StatProcessor> processor;

      //! shared pointer to a statistics publisher
      std::shared_ptr<ScalarStatPublisher> scalar_publisher;

      //! shared pointer to a statistics publisher
      std::shared_ptr<StatHdf5FileWriter> hdf5_publisher;

      //! shared pointer a statistics processor
      std::shared_ptr<ska::pst::smrb::SmrbSegmentProducer> producer;

      //! List of mandatory beam config keys
      const std::vector<std::string> beam_config_keys = {"DATA_KEY", "WEIGHTS_KEY"};

      //! List of mandatory scan config keys
      const std::vector<std::string> scan_config_keys = {"EB_ID", "STAT_PROC_DELAY_MS", "STAT_REQ_FREQ_BINS", "STAT_REQ_TIME_BINS", "STAT_NREBIN"};

      //! List of mandatory start scan config keys
      const std::vector<std::string> startscan_config_keys = {"SCAN_ID"};
  };

} // namespace ska::pst::stat

#endif // SKA_PST_STAT_StatApplicationManager_h
