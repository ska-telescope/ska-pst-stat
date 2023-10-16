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
#include <grpc++/grpc++.h>
#include <gtest/gtest.h>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/lmc/LmcService.h"
#include "ska/pst/stat/testutils/DataBlockTestHelper.h"

#include "ska/pst/stat/StatApplicationManager.h"
#include "ska/pst/stat/lmc/StatLmcServiceHandler.h"

#ifndef SKA_PST_STAT_TESTS_StatLmcServiceIntegrationTest_h
#define SKA_PST_STAT_TESTS_StatLmcServiceIntegrationTest_h

namespace ska::pst::stat::test {

/**
 * @brief Simple test to check integration via LmcService
 *
 * @details This will do a simple walk through the obs state lifecycle.
 *
 */
class StatLmcServiceIntegrationTest : public ::testing::Test
{
  protected:
    void SetUp() override;
    void TearDown() override;

    grpc::Status configure_beam(bool dry_run = false);
    grpc::Status configure_beam(const ska::pst::lmc::ConfigureBeamRequest& request);
    grpc::Status get_beam_configuration(ska::pst::lmc::GetBeamConfigurationResponse* response);
    grpc::Status deconfigure_beam();

    grpc::Status configure_scan(bool dry_run = false);
    grpc::Status deconfigure_scan();
    grpc::Status get_scan_configuration(ska::pst::lmc::GetScanConfigurationResponse* response);

    grpc::Status start_scan();
    grpc::Status stop_scan();

    grpc::Status abort();
    grpc::Status reset();
    grpc::Status restart();
    grpc::Status go_to_fault();

    grpc::Status get_state(ska::pst::lmc::GetStateResponse*);
    // set log level
    grpc::Status set_log_level(ska::pst::lmc::LogLevel required_log_level);
    grpc::Status get_log_level(ska::pst::lmc::GetLogLevelResponse& response);

    void assert_state(ska::pst::lmc::ObsState);
    void assert_manager_state(ska::pst::common::State);
    void assert_log_level(ska::pst::lmc::LogLevel);

  public:
    StatLmcServiceIntegrationTest();
    ~StatLmcServiceIntegrationTest() = default;

    // helper methods for common repeated code.
    void setup_data_block();
    void tear_down_data_block();
    void write_bytes_to_data_writer(uint64_t bytes_to_write);
    void write_bytes_to_weights_writer(uint64_t bytes_to_write);

    float delay_ms = 1000;
    int test_nblocks = 4;
    uint64_t data_bufsz;
    uint64_t weights_bufsz;
    std::string data_key;
    std::string weights_key;
    ska::pst::common::AsciiHeader beam_config;
    ska::pst::common::AsciiHeader scan_config;
    ska::pst::common::AsciiHeader start_scan_config;

    ska::pst::common::AsciiHeader data_scan_config;
    ska::pst::common::AsciiHeader weights_scan_config;

    ska::pst::common::AsciiHeader data_header;
    ska::pst::common::AsciiHeader weights_header;

    std::unique_ptr<DataBlockTestHelper> data_helper;
    std::unique_ptr<DataBlockTestHelper> weights_helper;

    int _port = 0;
    std::shared_ptr<ska::pst::common::LmcService> _service{nullptr};
    std::shared_ptr<ska::pst::stat::StatLmcServiceHandler> _handler{nullptr};
    std::shared_ptr<ska::pst::stat::StatApplicationManager> _stat{nullptr};
    std::shared_ptr<grpc::Channel> _channel{nullptr};
    std::shared_ptr<ska::pst::lmc::PstLmcService::Stub> _stub{nullptr};

    std::string stat_base_path = "/tmp";
};

} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatLmcServiceIntegrationTest_h
