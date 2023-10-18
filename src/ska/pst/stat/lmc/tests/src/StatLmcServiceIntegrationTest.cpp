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

#include <iostream>
#include <spdlog/spdlog.h>
#include <grpc/grpc.h>
#include <grpc++/grpc++.h>

#include "ska/pst/common/lmc/LmcService.h"

#include "ska/pst/stat/testutils/GtestMain.h"
#include "ska/pst/stat/lmc/tests/StatLmcServiceIntegrationTest.h"

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

static constexpr bool DRY_RUN = true;

StatLmcServiceIntegrationTest::StatLmcServiceIntegrationTest()
  : ::testing::Test()
{
}

void StatLmcServiceIntegrationTest::setup_data_block()
{
  weights_key = beam_config.get_val("WEIGHTS_KEY");
  data_key = beam_config.get_val("DATA_KEY");

  static uint64_t header_nbufs = beam_config.get_uint64("HB_NBUFS");
  static uint64_t header_bufsz = beam_config.get_uint64("HB_BUFSZ");
  static uint64_t data_nbufs = beam_config.get_uint64("DB_NBUFS");
  static uint64_t weights_nbufs = beam_config.get_uint64("WB_NBUFS");
  static constexpr uint64_t bufsz_factor = 16;
  static constexpr unsigned nreaders = 1;
  static constexpr int device = -1;

  weights_bufsz = weights_header.get_uint64("RESOLUTION") * bufsz_factor;
  data_bufsz = data_header.get_uint64("RESOLUTION") * bufsz_factor;

  weights_helper = std::make_unique<DataBlockTestHelper>(weights_key, 1);
  data_helper = std::make_unique<DataBlockTestHelper>(data_key, 1);

  weights_helper->set_data_block_bufsz(weights_bufsz);
  data_helper->set_data_block_bufsz(data_bufsz);

  weights_helper->set_data_block_nbufs(weights_nbufs);
  data_helper->set_data_block_nbufs(data_nbufs);

  weights_helper->set_header_block_nbufs(header_nbufs);
  data_helper->set_header_block_nbufs(header_nbufs);

  weights_helper->set_header_block_bufsz(header_bufsz);
  data_helper->set_header_block_bufsz(header_bufsz);

  weights_helper->set_config(weights_scan_config);
  data_helper->set_config(data_scan_config);

  weights_helper->set_header(weights_header);
  data_helper->set_header(data_header);

  weights_helper->setup();
  data_helper->setup();

  weights_helper->enable_reader();
  data_helper->enable_reader();

  weights_helper->start();
  data_helper->start();
  SPDLOG_TRACE("weights_helper->config:\n{}",weights_helper->config.raw());
  SPDLOG_TRACE("weights_helper->header:\n{}",weights_helper->header.raw());
  SPDLOG_TRACE("data_helper->config:\n{}",data_helper->config.raw());
  SPDLOG_TRACE("data_helper->header:\n{}",data_helper->header.raw());
}

void StatLmcServiceIntegrationTest::tear_down_data_block()
{
  SPDLOG_TRACE("tear_down_data_block teardown");
  data_helper->teardown();
  weights_helper->teardown();
  SPDLOG_TRACE("tear_down_data_block teardown complete");
}

void StatLmcServiceIntegrationTest::SetUp()
{
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::SetUp()");
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::SetUp creating shared data block");
  beam_config.load_from_file(test_data_file("beam_config.txt"));
  scan_config.load_from_file(test_data_file("scan_config.txt"));
  start_scan_config.load_from_file(test_data_file("start_scan_config.txt"));

  data_header.load_from_file(test_data_file("data_header_LowAA0.5.txt"));
  weights_header.load_from_file(test_data_file("weights_header_LowAA0.5.txt"));

  setup_data_block();
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::SetUp creating shared data block - finished");

  _stat = std::make_shared<ska::pst::stat::StatApplicationManager>(stat_base_path);

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::SetUp creating handler");
  _handler = std::make_shared<ska::pst::stat::StatLmcServiceHandler>(_stat);

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::SetUp creating service");
  _service = std::make_shared<ska::pst::common::LmcService>("test_integration_service", _handler, _port);

  // force getting a port set, we need gRPC to start to bind to get port.
  _service->start();

  _port = _service->port();

  std::string server_address("127.0.0.1:");
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::SetUp creating client connection on port {}", _port);
  server_address.append(std::to_string(_port));
  _channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
  _stub = ska::pst::lmc::PstLmcService::NewStub(_channel);
}

void StatLmcServiceIntegrationTest::TearDown()
{
  if (_stat->is_scanning())
  {
    _stat->stop_scan();
  }

  if (_stat->is_scan_configured())
  {
    _stat->deconfigure_scan();
  }

  if (_stat->is_beam_configured())
  {
    _stat->deconfigure_beam();
  }

  _service->stop();
}

auto StatLmcServiceIntegrationTest::configure_beam(bool dry_run) -> grpc::Status
{
  ska::pst::lmc::ConfigureBeamRequest request;
  request.set_dry_run(dry_run);
  auto beam_configuration = request.mutable_beam_configuration();
  auto *stat_resources_request = beam_configuration->mutable_stat();

  stat_resources_request->set_data_key(beam_config.get_val("DATA_KEY"));
  stat_resources_request->set_weights_key(beam_config.get_val("WEIGHTS_KEY"));

  return configure_beam(request);
}

auto StatLmcServiceIntegrationTest::configure_beam(const ska::pst::lmc::ConfigureBeamRequest& request) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::ConfigureBeamResponse response;

  return _stub->configure_beam(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::get_beam_configuration(
    ska::pst::lmc::GetBeamConfigurationResponse* response
) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::GetBeamConfigurationRequest request;

  return _stub->get_beam_configuration(&context, request, response);
}

auto StatLmcServiceIntegrationTest::deconfigure_beam() -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::DeconfigureBeamRequest request;
  ska::pst::lmc::DeconfigureBeamResponse response;

  return _stub->deconfigure_beam(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::configure_scan(bool dry_run) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::ConfigureScanRequest request;
  request.set_dry_run(dry_run);

  auto scan_configuration = request.mutable_scan_configuration();
  auto *stat_scan_configuration = scan_configuration->mutable_stat();

  stat_scan_configuration->set_execution_block_id(scan_config.get_val("EB_ID"));
  stat_scan_configuration->set_processing_delay_ms(scan_config.get_uint32("STAT_PROC_DELAY_MS"));
  stat_scan_configuration->set_req_time_bins(scan_config.get_uint32("STAT_REQ_TIME_BINS"));
  stat_scan_configuration->set_req_freq_bins(scan_config.get_uint32("STAT_REQ_FREQ_BINS"));
  stat_scan_configuration->set_num_rebin(scan_config.get_uint32("STAT_NREBIN"));

  ska::pst::lmc::ConfigureScanResponse response;

  return _stub->configure_scan(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::deconfigure_scan() -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::DeconfigureScanRequest request;
  ska::pst::lmc::DeconfigureScanResponse response;

  return _stub->deconfigure_scan(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::get_scan_configuration(
    ska::pst::lmc::GetScanConfigurationResponse *response
) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::GetScanConfigurationRequest request;

  return _stub->get_scan_configuration(&context, request, response);
}

auto StatLmcServiceIntegrationTest::start_scan() -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::StartScanRequest request;
  request.set_scan_id(start_scan_config.get_uint32("SCAN_ID"));
  ska::pst::lmc::StartScanResponse response;

  return _stub->start_scan(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::stop_scan() -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::StopScanRequest request;
  ska::pst::lmc::StopScanResponse response;

  return _stub->stop_scan(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::abort() -> grpc::Status
{
  grpc::ClientContext context; // NOLINT
  ska::pst::lmc::AbortRequest request; // NOLINT
  ska::pst::lmc::AbortResponse response; // NOLINT

  return _stub->abort(&context, request, &response); // NOLINT
}

auto StatLmcServiceIntegrationTest::reset() -> grpc::Status
{
  grpc::ClientContext context; // NOLINT
  ska::pst::lmc::ResetRequest request; // NOLINT
  ska::pst::lmc::ResetResponse response; // NOLINT

  return _stub->reset(&context, request, &response); // NOLINT
}

auto StatLmcServiceIntegrationTest::restart() -> grpc::Status
{
  grpc::ClientContext context; // NOLINT
  ska::pst::lmc::RestartRequest request; // NOLINT
  ska::pst::lmc::RestartResponse response; // NOLINT

  return _stub->restart(&context, request, &response); // NOLINT
}

auto StatLmcServiceIntegrationTest::go_to_fault() -> grpc::Status
{
  grpc::ClientContext context; // NOLINT
  ska::pst::lmc::GoToFaultRequest request; // NOLINT
  ska::pst::lmc::GoToFaultResponse response; // NOLINT

  return _stub->go_to_fault(&context, request, &response); // NOLINT
}


auto StatLmcServiceIntegrationTest::get_state(
  ska::pst::lmc::GetStateResponse* response
) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::GetStateRequest request;

  return _stub->get_state(&context, request, response);
}

void StatLmcServiceIntegrationTest::assert_log_level(
  ska::pst::lmc::LogLevel expected_loglevel
)
{
  switch (expected_loglevel)
  {
    case ska::pst::lmc::LogLevel::INFO:
      ASSERT_EQ(spdlog::level::info, spdlog::get_level());
      break;
    case ska::pst::lmc::LogLevel::DEBUG:
      ASSERT_EQ(spdlog::level::debug, spdlog::get_level());
      break;
    case ska::pst::lmc::LogLevel::WARNING:
      ASSERT_EQ(spdlog::level::warn, spdlog::get_level());
      break;
    case ska::pst::lmc::LogLevel::CRITICAL:
      ASSERT_EQ(spdlog::level::critical, spdlog::get_level());
      break;
    case ska::pst::lmc::LogLevel::ERROR:
      ASSERT_EQ(spdlog::level::err, spdlog::get_level());
      break;
    default:
      FAIL() << "LogLevel enum not implemented yet";
      break;
  }
}

void StatLmcServiceIntegrationTest::assert_state(
    ska::pst::lmc::ObsState expected_state
)
{
  ska::pst::lmc::GetStateResponse get_state_response; // NOLINT
  auto status = get_state(&get_state_response); // NOLINT
  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_EQ(get_state_response.state(), expected_state); // NOLINT

  switch (expected_state)
  {
    case ska::pst::lmc::ObsState::EMPTY:
      assert_manager_state(ska::pst::common::Idle);
      break;
    case ska::pst::lmc::ObsState::IDLE:
      assert_manager_state(ska::pst::common::BeamConfigured);
      break;
    case ska::pst::lmc::ObsState::READY:
      assert_manager_state(ska::pst::common::ScanConfigured);
      break;
    case ska::pst::lmc::ObsState::SCANNING:
      assert_manager_state(ska::pst::common::Scanning);
      break;
    case ska::pst::lmc::ObsState::FAULT:
      assert_manager_state(ska::pst::common::RuntimeError);
      break;
    default: // NOLINT
      // we can't assert the manager state for other expected states
      break;
  }
}

auto StatLmcServiceIntegrationTest::set_log_level(
    ska::pst::lmc::LogLevel required_log_level
) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::SetLogLevelResponse response;
  ska::pst::lmc::SetLogLevelRequest request;
  request.set_log_level(required_log_level);
  return _stub->set_log_level(&context, request, &response);
}

auto StatLmcServiceIntegrationTest::get_log_level(
  ska::pst::lmc::GetLogLevelResponse& response
) -> grpc::Status
{
  grpc::ClientContext context;
  ska::pst::lmc::GetLogLevelRequest request;
  return _stub->get_log_level(&context, request, &response);
}

void StatLmcServiceIntegrationTest::assert_manager_state(
  ska::pst::common::State expected_state
)
{
  auto curr_state = _stat->get_state();
  EXPECT_EQ(curr_state, expected_state); // NOLINT
}

TEST_F(StatLmcServiceIntegrationTest, start_scan_stop_scan) // NOLINT
{
  EXPECT_TRUE(_service->is_running()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::EMPTY);

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - configuring beam");
  auto status = configure_beam();

  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_TRUE(_stat->is_beam_configured()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::IDLE);
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - beam configured");

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - checking beam configuration");
  ska::pst::lmc::GetBeamConfigurationResponse beam_configuration_response;
  status = get_beam_configuration(&beam_configuration_response);
  EXPECT_TRUE(status.ok()); // NOLINT
  const auto &beam_configuration = beam_configuration_response.beam_configuration();
  EXPECT_TRUE(beam_configuration.has_stat());
  const auto &stat_beam_configuration = beam_configuration.stat();
  EXPECT_EQ(stat_beam_configuration.data_key(), beam_config.get_val("DATA_KEY"));
  EXPECT_EQ(stat_beam_configuration.weights_key(), beam_config.get_val("WEIGHTS_KEY"));
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - beam configuration checked");

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - configuring scan");
  status = configure_scan();
  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_TRUE(_stat->is_scan_configured()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::READY);
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - scan configured");

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - checking scan configuration");
  ska::pst::lmc::GetScanConfigurationResponse scan_configuration_response;
  status = get_scan_configuration(&scan_configuration_response);
  EXPECT_TRUE(status.ok()); // NOLINT
  const auto &scan_configuration = scan_configuration_response.scan_configuration();
  EXPECT_TRUE(scan_configuration.has_stat());
  const auto &stat_scan_configuration = scan_configuration.stat();
  EXPECT_EQ(stat_scan_configuration.execution_block_id(), scan_config.get_val("EB_ID"));
  EXPECT_EQ(stat_scan_configuration.processing_delay_ms(), scan_config.get_uint32("STAT_PROC_DELAY_MS"));
  EXPECT_EQ(stat_scan_configuration.req_time_bins(), scan_config.get_uint32("STAT_REQ_TIME_BINS"));
  EXPECT_EQ(stat_scan_configuration.req_freq_bins(), scan_config.get_uint32("STAT_REQ_FREQ_BINS"));
  EXPECT_EQ(stat_scan_configuration.num_rebin(), scan_config.get_uint32("STAT_NREBIN"));
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - scan configuration checked");

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - starting scan");
  status = start_scan();
  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_TRUE(_stat->is_scanning()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::SCANNING);
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - scanning");

  // sleep to allow scan to do its job
  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();
  usleep(ska::pst::common::microseconds_per_decisecond);

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - ending scan");
  status = stop_scan();
  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_FALSE(_stat->is_scanning()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::READY);
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - scan ended");

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - deconfiguring scan");
  status = deconfigure_scan();
  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_FALSE(_stat->is_scan_configured()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::IDLE);
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - scan deconfigured");

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - deconfiguring beam");
  status = deconfigure_beam();
  EXPECT_TRUE(status.ok()); // NOLINT
  EXPECT_FALSE(_stat->is_beam_configured()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::EMPTY);
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::start_scan_stop_scan - beam deconfigured");
}

TEST_F(StatLmcServiceIntegrationTest, go_to_fault_when_runtime_error_encountered_configure_beam) // NOLINT
{
  // Induce error in config to simulate RuntimeError
  beam_config.set_val("DATA_KEY","!@#$%");
  _stat->set_timeout(1);

  EXPECT_TRUE(_service->is_running()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::EMPTY); // NOLINT

  auto status = configure_beam(); // NOLINT
  EXPECT_FALSE(status.ok()); // NOLINT
  EXPECT_FALSE(_stat->is_beam_configured()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::FAULT); // NOLINT
}

TEST_F(StatLmcServiceIntegrationTest, go_to_idle_when_reset) // NOLINT
{
  // Induce error in config to simulate RuntimeError
  beam_config.set_val("DATA_KEY","!@#$%");
  _stat->set_timeout(1);

  EXPECT_TRUE(_service->is_running()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::EMPTY); // NOLINT

  auto status = configure_beam(); // NOLINT
  EXPECT_FALSE(status.ok()); // NOLINT
  EXPECT_FALSE(_stat->is_beam_configured()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::FAULT); // NOLINT
  assert_state(ska::pst::lmc::ObsState::FAULT); // NOLINT
  status = reset();
  assert_state(ska::pst::lmc::ObsState::EMPTY);
}

TEST_F(StatLmcServiceIntegrationTest, reset_when_in_fault_state) // NOLINT
{
  EXPECT_TRUE(_service->is_running()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::EMPTY); // NOLINT

  auto status = go_to_fault();
  EXPECT_TRUE(status.ok()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::FAULT); // NOLINT

  status = reset();
  assert_state(ska::pst::lmc::ObsState::EMPTY);
}

TEST_F(StatLmcServiceIntegrationTest, validate_configure_scan) // NOLINT
{
  EXPECT_TRUE(_service->is_running()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::EMPTY); // NOLINT

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::validate_configure_scan - configuring beam"); // NOLINT
  auto status = configure_beam(); // NOLINT
  assert_state(ska::pst::lmc::ObsState::IDLE); // NOLINT
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::validate_configure_scan - beam configured"); // NOLINT

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::validate_configure_scan - validating scan configuration"); // NOLINT
  status = configure_scan(DRY_RUN); // NOLINT
  EXPECT_TRUE(status.ok()); // NOLINT
  assert_state(ska::pst::lmc::ObsState::IDLE); // NOLINT
  SPDLOG_TRACE("StatLmcServiceIntegrationTest::validate_configure_scan - scan config validated"); // NOLINT
}

TEST_F(StatLmcServiceIntegrationTest, set_log_levels) // NOLINT
{
  EXPECT_TRUE(_service->is_running()); // NOLINT
  ska::pst::lmc::GetLogLevelResponse response;

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::set_loglevel - DEBUG"); // NOLINT
  set_log_level(ska::pst::lmc::LogLevel::DEBUG);
  assert_log_level(ska::pst::lmc::LogLevel::DEBUG);
  auto status = get_log_level(response);
  EXPECT_TRUE(status.ok());
  ASSERT_EQ(ska::pst::lmc::LogLevel::DEBUG, response.log_level());

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::set_log_level - INFO"); // NOLINT
  set_log_level(ska::pst::lmc::LogLevel::INFO);
  assert_log_level(ska::pst::lmc::LogLevel::INFO);
  status = get_log_level(response);
  EXPECT_TRUE(status.ok());
  ASSERT_EQ(ska::pst::lmc::LogLevel::INFO, response.log_level());

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::set_log_level - WARNING"); // NOLINT
  set_log_level(ska::pst::lmc::LogLevel::WARNING);
  assert_log_level(ska::pst::lmc::LogLevel::WARNING);
  status = get_log_level(response);
  EXPECT_TRUE(status.ok());
  ASSERT_EQ(ska::pst::lmc::LogLevel::WARNING, response.log_level());

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::set_log_level - CRITICAL"); // NOLINT
  set_log_level(ska::pst::lmc::LogLevel::CRITICAL);
  assert_log_level(ska::pst::lmc::LogLevel::CRITICAL);
  status = get_log_level(response);
  EXPECT_TRUE(status.ok());
  ASSERT_EQ(ska::pst::lmc::LogLevel::CRITICAL, response.log_level());

  SPDLOG_TRACE("StatLmcServiceIntegrationTest::set_log_level - ERROR"); // NOLINT
  set_log_level(ska::pst::lmc::LogLevel::ERROR);
  assert_log_level(ska::pst::lmc::LogLevel::ERROR);
  status = get_log_level(response);
  EXPECT_TRUE(status.ok());
  ASSERT_EQ(ska::pst::lmc::LogLevel::ERROR, response.log_level());
}

} // namespace ska::pst::stat::test
