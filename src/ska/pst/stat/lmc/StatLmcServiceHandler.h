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

#ifndef __SKA_PST_StatLmcServiceHandler_h
#define __SKA_PST_StatLmcServiceHandler_h

#include <memory>

#include "ska/pst/stat/StatApplicationManager.h"
#include "ska/pst/common/lmc/LmcServiceHandler.h"

namespace ska::pst::stat {

    /**
     * @brief Class to act as a bridge between the local monitoring and control
     *    service of STAT.CORE and the \ref ska::pst::stat::StatApplicationManager
     *
     */
    class StatLmcServiceHandler final : public ska::pst::common::LmcServiceHandler {
        private:
            /**
             * @brief A share pointer to a \ref ska::pst::stat::StatApplicationManager
             *      which is used to manage SKA.PST.STAT functionality.
             */
            std::shared_ptr<ska::pst::stat::StatApplicationManager> stat;

        public:
            StatLmcServiceHandler(std::shared_ptr<ska::pst::stat::StatApplicationManager> stat): stat(std::move(stat)) {}
            virtual ~StatLmcServiceHandler() = default;

            // validation methods
            /**
             * @brief Validate a beam configuration.
             *
             * Validate a beam configuration for correctness but does not apply the configuration.
             *
             * @throw std::exception if there is a problem with the beam configuration of the service.
             * @throw ska::pst::common::pst_validation_error if there are validation errors in the request.
             */
            void validate_beam_configuration(const ska::pst::lmc::BeamConfiguration &configuration) override;

            /**
             * @brief Validate a scan configuration.
             *
             * Validate a scan configuration for correctness but does not apply the configuration.
             *
             * @throw std::exception if there is a problem with the beam configuration of the service.
             * @throw ska::pst::common::pst_validation_error if there are validation errors in the request.
             */
            void validate_scan_configuration(const ska::pst::lmc::ScanConfiguration &configuration) override;

            // beam resourcing methods
            /**
             * @brief Handle configuring the service to be a part of a beam.
             *
             * This implementation expects that there is an stst sub-field in the resources
             * request and calls the \ref StatApplicationManager.configure_beam method, which expects
             * keys for the data and weights ring buffers.
             *
             * @param configuration the configuration for the beam. This message has oneof field should
             *      be the stst sub-field message.
             * @throws ska::pst::common::LmcServiceException if resources had already been assigned.
             */
            void configure_beam(const ska::pst::lmc::BeamConfiguration &resources) override;

            /**
             * @brief Handle deconfiguring the service from a beam.
             *
             * This calles the \ref StatApplicationManager.deconfigure_beam to detach from the ring buffers.
             * This method checks to see the manager has beam configuration.
             *
             * @throws ska::pst::common::LmcServiceException if manager is not beam configured.
             */
            void deconfigure_beam() override;

            /**
             * @brief Handle getting the current beam configuration for the service.
             *
             * This will return the current beam configuration for the disk manager.
             *
             * @param resources the out protobuf message to used to return beam configuration details.
             * @throws ska::pst::common::LmcServiceException if manager is not beam configured.
             */
            void get_beam_configuration(ska::pst::lmc::BeamConfiguration* resources) override;

            /**
             * @brief Check if this service is configured for a beam.
             *
             * Will return true if the \ref StatApplicationManager has beam configuration.
             */
            bool is_beam_configured() const noexcept override;

            // scan configuration methods
            /**
             * @brief Handle configuring the service for a scan.
             *
             * This will configure the \ref StatApplicationManager for a scan. This implementation
             * expects that the stst sub-message is set on the configuration parameter.
             *
             * @param configuration a protobuf message that should have the stst sub-message set.
             * @throws ska::pst::common::LmcServiceException if manager is not beam configured,
             *      configuration parameter doesn't have a a stst sub-message, or that the
             *      disk manager is already configured for a scan.
             */
            void configure_scan(const ska::pst::lmc::ScanConfiguration &configuration) override;

            /**
             * @brief Handle deconfiguring service for a scan.
             *
             * This will deconfigure the \ref StatApplicationManager for a scan.
             *
             * @throws ska::pst::common::LmcServiceException if manager is configured for a scan.
             */
            void deconfigure_scan() override;

            /**
             * @brief Handle getting the current scan configuration for the service.
             *
             * This will return a stst sub-field message with the current scan configuration.
             *
             * @param configuration the out protobuf message to used to return scan configuration details.
             * @throws ska::pst::common::LmcServiceException if manager is configured for a scan.
             */
            void get_scan_configuration(ska::pst::lmc::ScanConfiguration *configuration) override;

            /**
             * @brief Check if the service has been configured for a scan.
             *
             * @return This will return true if \ref StatApplicationManager is configured for a scan.
             */
            bool is_scan_configured() const noexcept override;

            // scan method
            /**
             * @brief Handle initiating a scan.
             *
             * This will call \ref StatApplicationManager.start_scan that will
             * initial a scan, including reading the ring buffers and writing
             * voltages to disk.
             *
             * @throws ska::pst::common::LmcServiceException if manager is configured for a scan, or
             *      is already scanning.
             */
            void start_scan(const ska::pst::lmc::StartScanRequest &request) override;

            /**
             * @brief Handle ending a scan.
             *
             * This will call \ref StatApplicationManager.stop_scan if the service is
             * already scanning.
             *
             * @throws ska::pst::common::LmcServiceException if service is not scanning.
             */
            void stop_scan() override;

            /**
             * @brief Handle resetting State into Idle
             *
             * This ties the states between LmcService ObsState::EMPTY with ApplicationManager State::Idle
             */
            void reset() override;

            /**
             * @brief Check if the service is currenting performing a scan.
             *
             * @return true if the \ref DiskManger is scanning.
             */
            bool is_scanning() const noexcept override;

            // monitoring
            /**
             * @brief Handle getting the monitoring data for the service.
             *
             * This will get the current monitoring data for STAT.CORE. This includes
             * the total capacity, available capacity, the amount of bytes written in as
             * scan, and the current write rate.
             *
             * @param data Pointer to the protobuf message to return. This will set the stat
             *      sub-message with the monitoring data.
             * @throws ska::pst::common::LmcServiceException if service is not scanning.
             */
            void get_monitor_data(ska::pst::lmc::MonitorData *data) override;

            /**
             * @brief Return environment variables back to the client.
             *
             * This implementation returns disk_capacity and disk_available_bytes as
             * unsigned int values.  The disk_capacity is the total size of the volumne
             * while disk_available_bytes is amount of disk space left on the volumne
             *
             * @param data Pointer to a protobuf message message that includes the a map to populate.
             */
            void get_env(ska::pst::lmc::GetEnvironmentResponse *response) noexcept override;

            /**
             * @brief Get the StatApplicationManager state
             *
             * @return ska::pst::common::State returns current the enum State of the StatApplicationManager
             */
            ska::pst::common::State get_application_manager_state() { return stat->get_state(); }

            /**
             * @brief Get the StatApplicationManager exception pointer
             *
             * @return std::exception_ptr returns the current captured exception caught by the StatApplicationManager
             */
            std::exception_ptr get_application_manager_exception() { return stat->get_exception(); }

            /**
              * @brief Put application into a runtime error state.
              *
              * @param exception an exception pointer to store on the application manager.
             */
            void go_to_runtime_error(std::exception_ptr exc) override;
    };

} // namespace ska::pst::stat

#endif // __SKA_PST_StatLmcServiceHandler_h
