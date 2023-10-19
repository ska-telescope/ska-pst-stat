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

#include "ska/pst/stat/testutils/GtestMain.h"
#include "ska/pst/common/lmc/LmcServiceException.h"

#include "ska/pst/stat/lmc/tests/StatLmcServiceHandlerTest.h"

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatLmcServiceHandlerTest::StatLmcServiceHandlerTest()
  : ::testing::Test()
{
}

void StatLmcServiceHandlerTest::setup_data_block()
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

void StatLmcServiceHandlerTest::tear_down_data_block()
{
  SPDLOG_TRACE("tear_down_data_block teardown");
  data_helper->teardown();
  weights_helper->teardown();
  SPDLOG_TRACE("tear_down_data_block teardown complete");
}

void StatLmcServiceHandlerTest::SetUp()
{
  beam_config.load_from_file(test_data_file("beam_config.txt"));
  scan_config.load_from_file(test_data_file("scan_config.txt"));
  start_scan_config.load_from_file(test_data_file("start_scan_config.txt"));

  data_header.load_from_file(test_data_file("data_header_LowAA0.5.txt"));
  weights_header.load_from_file(test_data_file("weights_header_LowAA0.5.txt"));

  setup_data_block();

  SPDLOG_DEBUG("StatLmcServiceHandlerTest::SetUp()");
  SPDLOG_DEBUG("StatLmcServiceHandlerTest::SetUp creating stat application manager");
  _stat = std::make_shared<ska::pst::stat::StatApplicationManager>(stat_base_path);

  SPDLOG_DEBUG("StatLmcServiceHandlerTest::SetUp creating handler");
  handler = std::make_shared<ska::pst::stat::StatLmcServiceHandler>(_stat);
}

void StatLmcServiceHandlerTest::TearDown()
{
  SPDLOG_DEBUG("StatLmcServiceHandlerTest::TearDown()");
  tear_down_data_block();
}

void StatLmcServiceHandlerTest::configure_beam()
{
  ska::pst::lmc::BeamConfiguration request;
  auto stat_resources_request = request.mutable_stat();
  SPDLOG_DEBUG("beam_config: {}", beam_config.raw());
  stat_resources_request->set_data_key(beam_config.get_val("DATA_KEY"));
  stat_resources_request->set_weights_key(beam_config.get_val("WEIGHTS_KEY"));
  handler->configure_beam(request);
}

void StatLmcServiceHandlerTest::configure_scan()
{
  ska::pst::lmc::ScanConfiguration request;
  auto stat_resources_request = request.mutable_stat();
  stat_resources_request->set_execution_block_id(scan_config.get_val("EB_ID"));
  stat_resources_request->set_processing_delay_ms(scan_config.get_uint32("STAT_PROC_DELAY_MS"));
  stat_resources_request->set_req_time_bins(scan_config.get_uint32("STAT_REQ_TIME_BINS"));
  stat_resources_request->set_req_freq_bins(scan_config.get_uint32("STAT_REQ_FREQ_BINS"));
  stat_resources_request->set_num_rebin(scan_config.get_uint32("STAT_NREBIN"));
  handler->configure_scan(request);
}

void StatLmcServiceHandlerTest::start_scan()
{
  ska::pst::lmc::StartScanRequest request;
  request.set_scan_id(start_scan_config.get_uint32("SCAN_ID"));
  handler->start_scan(request);
}

TEST_F(StatLmcServiceHandlerTest, configure_deconfigure_beam) // NOLINT
{
  SPDLOG_DEBUG("StatLmcServiceHandlerTest::configure_deconfigure_beam - configuring beam");
  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT

  EXPECT_FALSE(_stat->is_beam_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_beam - configure_beam");

  configure_beam();

  EXPECT_TRUE(handler->is_beam_configured()); // NOLINT
  EXPECT_TRUE(_stat->is_beam_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_beam - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_beam - getting beam configuration");
  ska::pst::lmc::BeamConfiguration beam_configuration;
  handler->get_beam_configuration(&beam_configuration);

  EXPECT_TRUE(beam_configuration.has_stat()); // NOLINT
  auto stat_beam_configuration = beam_configuration.stat();


  EXPECT_EQ(data_key, stat_beam_configuration.data_key()); // NOLINT
  EXPECT_EQ(weights_key, stat_beam_configuration.weights_key()); // NOLINT

  ska::pst::common::AsciiHeader &stat_resources = _stat->get_beam_configuration();
  EXPECT_EQ(data_key, stat_resources.get_val("DATA_KEY")); // NOLINT
  EXPECT_EQ(weights_key, stat_resources.get_val("WEIGHTS_KEY")); // NOLINT
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_beam - checked beam configuration");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_beam - deconfiguring beam");
  handler->deconfigure_beam();
  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT
  EXPECT_FALSE(_stat->is_beam_configured()); // NOLINT
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_beam - beam deconfigured");
}

TEST_F(StatLmcServiceHandlerTest, configure_beam_again_should_throw_exception) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_again_should_throw_exception - configuring beam");

  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT

  configure_beam();

  EXPECT_TRUE(handler->is_beam_configured()); // NOLINT
  EXPECT_TRUE(_stat->is_beam_configured()); // NOLINT
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_again_should_throw_exception - beam configured");

  try
  {
    configure_beam();
    FAIL() << " expected configure_beam to throw exception due to beam configured already.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Beam already configured for STAT.CORE.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::CONFIGURED_FOR_BEAM_ALREADY);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, configure_beam_with_validation_errors) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_with_validation_errors - configuring beam");

  beam_config.del("DATA_KEY");
  _stat->set_timeout(1);

  EXPECT_FALSE(handler->is_beam_configured());
  try
  {
    configure_beam();
    FAIL() << " expected configure_beam to throw exception due to invalid configuration.\n";
  }
  catch (std::exception& ex)
  {
    SPDLOG_DEBUG("StatLmcServiceHandlerTest::configure_beam_with_validation_errors exception thrown as expected: {}", ex.what());
    EXPECT_EQ(_stat->get_state(), ska::pst::common::State::Idle);
    EXPECT_FALSE(handler->is_beam_configured());
  }
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_with_validation_errors test done");
}

TEST_F(StatLmcServiceHandlerTest, configure_beam_with_invalid_configuration) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_with_invalid_configuration - configuring beam");

  beam_config.set_val("DATA_KEY", "abcdefg");
  _stat->set_timeout(1);

  EXPECT_FALSE(handler->is_beam_configured());
  try
  {
    configure_beam();
    FAIL() << " expected configure_beam to throw exception due to invalid configuration.\n";
  }
  catch (std::exception& ex)
  {
    SPDLOG_DEBUG("StatLmcServiceHandlerTest::configure_beam_with_invalid_configuration exception thrown as expected: {}", ex.what());
    EXPECT_EQ(_stat->get_state(), ska::pst::common::State::RuntimeError);
    EXPECT_FALSE(handler->is_beam_configured());
  }
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_with_invalid_configuration test done");
}

TEST_F(StatLmcServiceHandlerTest, configure_beam_should_have_stat_object) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_beam_should_have_stat_object - configuring beam");
  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT

  ska::pst::lmc::BeamConfiguration beam_configuration;
  beam_configuration.mutable_test();

  try
  {
    handler->configure_beam(beam_configuration);
    FAIL() << " expected configure_beam to throw exception due not having STAT.CORE field.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Expected a STAT.CORE beam configuration object, but none were provided.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::INVALID_REQUEST);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::INVALID_ARGUMENT);
  }
}

TEST_F(StatLmcServiceHandlerTest, get_beam_configuration_when_not_beam_configured) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_beam_configuration_when_not_beam_configured - getting beam configuration");
  EXPECT_FALSE(handler->is_beam_configured());

  ska::pst::lmc::BeamConfiguration response;
  try
  {
    handler->get_beam_configuration(&response);
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "STAT.CORE not configured for beam.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_BEAM);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_beam_configuration_when_not_beam_configured - beam not configured");
}

TEST_F(StatLmcServiceHandlerTest, deconfigure_beam_when_not_beam_configured) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_not_beam_configured - deconfiguring beam");

  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT

  try
  {
    handler->deconfigure_beam();
    FAIL() << " expected deconfigure_beam to throw exception due to beam not configured.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "STAT.CORE not configured for beam.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_BEAM);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, deconfigure_beam_when_scan_configured) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_scan_configured - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_scan_configured - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_scan_configured - configuring scan");
  EXPECT_FALSE(handler->is_scan_configured());
  configure_scan();
  EXPECT_TRUE(handler->is_scan_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_scan_configured - scan configured");

  try
  {
    handler->deconfigure_beam();
    FAIL() << " expected deconfigure_beam to throw exception due having scan configuration.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_scan_configured exception occured as expected");
    EXPECT_EQ(std::string(ex.what()), "STAT.CORE is configured for scan but trying to deconfigure beam.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::INVALID_REQUEST);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_beam_when_scan_configured done");
}

TEST_F(StatLmcServiceHandlerTest, configure_deconfigure_scan) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - configuring scan");
  EXPECT_FALSE(handler->is_scan_configured());

  configure_scan();
  EXPECT_TRUE(handler->is_scan_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - scan configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - getting configuration");
  ska::pst::lmc::ScanConfiguration get_response;
  handler->get_scan_configuration(&get_response);
  EXPECT_TRUE(get_response.has_stat()); // NOLINT
  const auto &stat_scan_configuration = get_response.stat();

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - checked configuration");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - deconfiguring scan");
  handler->deconfigure_scan();
  EXPECT_FALSE(handler->is_scan_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - scan deconfigured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - deconfiguring beam");
  handler->deconfigure_beam();
  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_deconfigure_scan - beam deconfigured");
}

TEST_F(StatLmcServiceHandlerTest, configure_scan_with_invalid_configuration) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_with_invalid_configuration - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_with_invalid_configuration - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_with_invalid_configuration - configuring scan");
  EXPECT_FALSE(handler->is_scan_configured());

  scan_config.del("EB_ID");
  try
  {
    configure_scan();
    FAIL() << " expected configure_scan to throw exception due to not having beam configured.\n";
  } catch (std::exception& ex) {
    EXPECT_EQ(_stat->get_state(), ska::pst::common::State::BeamConfigured);
    EXPECT_FALSE(handler->is_scan_configured());
    EXPECT_TRUE(handler->is_beam_configured());

    SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_with_invalid_configuration deconfiguring beam");
    handler->deconfigure_beam();
    EXPECT_EQ(_stat->get_state(), ska::pst::common::State::Idle);
    SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_with_invalid_configuration deconfiguring beam");
  }

}

TEST_F(StatLmcServiceHandlerTest, configure_scan_when_not_beam_configured) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_when_not_beam_configured - configuring scan");

  try
  {
    configure_scan();
    FAIL() << " expected configure_scan to throw exception due to not having beam configured.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "STAT.CORE not configured for beam.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_BEAM);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, configure_scan_again_should_throw_exception) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_again_should_throw_exception - configuring beam");
  EXPECT_FALSE(handler->is_beam_configured()); // NOLINT
  configure_beam();
  EXPECT_TRUE(handler->is_beam_configured()); // NOLINT
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_again_should_throw_exception - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_again_should_throw_exception - configuring scan");
  EXPECT_FALSE(handler->is_scan_configured());
  configure_scan();
  EXPECT_TRUE(handler->is_scan_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_again_should_throw_exception - scan configured");

  try
  {
    configure_scan();
    FAIL() << " expected configure_scan to throw exception due to scan already configured.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Scan already configured for STAT.CORE.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::CONFIGURED_FOR_SCAN_ALREADY);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, configure_scan_should_have_stat_object) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_should_have_stat_object - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_should_have_stat_object - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::configure_scan_should_have_stat_object - configuring scan");
  try
  {
    ska::pst::lmc::ScanConfiguration configuration;
    configuration.mutable_test();
    handler->configure_scan(configuration);
    FAIL() << " expected configure_scan to throw exception due not having STAT.CORE field.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Expected a STAT.CORE scan configuration object, but none were provided.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::INVALID_REQUEST);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::INVALID_ARGUMENT);
  }
}

TEST_F(StatLmcServiceHandlerTest, deconfigure_scan_when_not_configured) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::deconfigure_scan_when_not_configured - deconfiguring scan");

  try
  {
    handler->deconfigure_scan();
    FAIL() << " expected deconfigure_scan to throw exception due to beam not configured.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Scan not currently configured for STAT.CORE.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_SCAN);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, get_scan_configuration_when_not_configured) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_scan_configuration_when_not_configured - getting scan configuration"); // NOLINT

  try
  {
    ska::pst::lmc::ScanConfiguration scan_configuration;
    handler->get_scan_configuration(&scan_configuration);
    FAIL() << " expected deconfigure_beam to throw exception due to beam not configured.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Scan Not currently configured for STAT.CORE.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_SCAN);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, start_scan_stop_scan) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - configuring scan");
  configure_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - scan configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - starting scan");
  EXPECT_FALSE(handler->is_scanning());
  start_scan();
  EXPECT_TRUE(handler->is_scanning());

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - scanning");

  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  // sleep for bit, want to wait so we can stop.
  usleep(ska::pst::common::microseconds_per_decisecond);

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - ending scan");
  handler->stop_scan();
  EXPECT_FALSE(handler->is_scanning());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - scan ended");
}

TEST_F(StatLmcServiceHandlerTest, start_scan_when_scanning_should_throw_exception) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_scanning - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_scanning - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_scanning - configuring scan");
  configure_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_scanning - scan configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_scanning - starting scan");
  EXPECT_FALSE(handler->is_scanning());
  start_scan();
  EXPECT_TRUE(handler->is_scanning());

  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  // sleep for bit, want to wait so we can stop.
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_stop_scan - scanning");
  usleep(ska::pst::common::microseconds_per_decisecond);

  try
  {
    start_scan();
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "STAT.CORE is already scanning.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::ALREADY_SCANNING);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, start_scan_when_not_configured_should_throw_exception) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_not_configured_should_throw_exception - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_not_configured_should_throw_exception - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::start_scan_when_not_configured_should_throw_exception - start scan");
  EXPECT_FALSE(handler->is_scan_configured());

  try
  {
    start_scan();
    FAIL() << " expected start_scan to throw exception due to scan not being configured.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Scan not currently configured for STAT.CORE.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_SCAN);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, stop_scan_when_not_scanning_should_throw_exception) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::stop_scan_when_not_scanning_should_throw_exception - end scan");
  EXPECT_FALSE(handler->is_scanning());

  try
  {
    handler->stop_scan();
    FAIL() << " expected stop_scan to throw exception due to not currently scanning.\n";
  }
  catch (ska::pst::common::LmcServiceException& ex)
  {
    EXPECT_EQ(std::string(ex.what()), "Received stop_scan request when STAT.CORE is not scanning.");
    EXPECT_EQ(ex.error_code(), ska::pst::lmc::ErrorCode::NOT_SCANNING);
    EXPECT_EQ(ex.status_code(), grpc::StatusCode::FAILED_PRECONDITION);
  }
}

TEST_F(StatLmcServiceHandlerTest, get_monitor_data) // NOLINT
{
  // configure beam
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - beam configured");

  // configure
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - configuring scan");
  configure_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - scan configured");


  // scan
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - starting scan");
  EXPECT_FALSE(handler->is_scanning());
  start_scan();
  EXPECT_TRUE(handler->is_scanning());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - scanning");

  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  static constexpr uint64_t max_to_wait = 60 * ska::pst::common::microseconds_per_second;
  unsigned waited_so_far = 0;

  const auto &scalar_stats = _stat->get_scalar_stats();
  SPDLOG_INFO("StatLmcServiceHandlerTest::get_monitor_data scalar_stats.mean_frequency_avg={}", scalar_stats.mean_frequency_avg.size());
  ska::pst::lmc::MonitorData monitor_data;
  handler->get_monitor_data(&monitor_data);
  EXPECT_TRUE(monitor_data.has_stat()); // NOLINT

  SPDLOG_INFO("StatLmcServiceHandlerTest::get_monitor_data - _stat->get_scalar_stats");
  const auto &stat_monitor_data = monitor_data.stat();
  SPDLOG_INFO("StatLmcServiceHandlerTest::get_monitor_data stat_monitor_data.mean_frequency_avg={}", stat_monitor_data.mean_frequency_avg().size());

  EXPECT_NO_THROW(stat_monitor_data.mean_frequency_avg()); // NOLINT
  EXPECT_NO_THROW(stat_monitor_data.mean_frequency_avg_masked()); // NOLINT
  EXPECT_NO_THROW(stat_monitor_data.variance_frequency_avg()); // NOLINT
  EXPECT_NO_THROW(stat_monitor_data.variance_frequency_avg_masked()); // NOLINT
  EXPECT_NO_THROW(stat_monitor_data.num_clipped_samples()); // NOLINT
  EXPECT_NO_THROW(stat_monitor_data.num_clipped_samples_masked()); // NOLINT

  // end scan
  usleep(ska::pst::common::microseconds_per_decisecond);
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - ending scan");
  handler->stop_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - scan ended");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - deconfiguring scan");
  handler->deconfigure_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - scan deconfigured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - deconfiguring beam");
  handler->deconfigure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_monitor_data - beam deconfigured");
}

TEST_F(StatLmcServiceHandlerTest, controlled_get_monitor_data) // NOLINT
{
  // configure beam
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - beam configured");

  // configure
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - configuring scan");
  configure_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - scan configured");


  // scan
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - starting scan");
  EXPECT_FALSE(handler->is_scanning());
  start_scan();
  EXPECT_TRUE(handler->is_scanning());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - scanning");

  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  static constexpr uint64_t max_to_wait = 60 * ska::pst::common::microseconds_per_second;
  unsigned waited_so_far = 0;

  while (_stat->get_processing_state() == ska::pst::stat::StatApplicationManager::ProcessingState::Processing && waited_so_far < max_to_wait)
  {
    usleep(ska::pst::common::microseconds_per_decisecond);
    waited_so_far += ska::pst::common::microseconds_per_decisecond;
  }

  const auto &scalar_stats = _stat->get_scalar_stats();
  SPDLOG_INFO("StatLmcServiceHandlerTest::controlled_get_monitor_data scalar_stats.mean_frequency_avg={}", scalar_stats.mean_frequency_avg.size());
  ska::pst::lmc::MonitorData monitor_data;
  handler->get_monitor_data(&monitor_data);
  EXPECT_TRUE(monitor_data.has_stat()); // NOLINT

  SPDLOG_INFO("StatLmcServiceHandlerTest::controlled_get_monitor_data - _stat->get_scalar_stats");
  const auto &stat_monitor_data = monitor_data.stat();
  SPDLOG_INFO("StatLmcServiceHandlerTest::controlled_get_monitor_data stat_monitor_data.mean_frequency_avg={}", stat_monitor_data.mean_frequency_avg().size());

  int index_data_size=0;
  SPDLOG_TRACE("scalar_stats.mean_frequency_avg.size()={}", scalar_stats.mean_frequency_avg.size());
  for (int i_npol=0; i_npol<scalar_stats.mean_frequency_avg.size(); i_npol++)
  {
    SPDLOG_TRACE("scalar_stats.mean_frequency_avg[{}].size()={}", i_npol, scalar_stats.mean_frequency_avg[i_npol].size());
    for (int i_ndim=0; i_ndim<scalar_stats.mean_frequency_avg[i_npol].size(); i_ndim++)
    {
      EXPECT_EQ(stat_monitor_data.mean_frequency_avg()[index_data_size], scalar_stats.mean_frequency_avg[i_npol][i_ndim]); // NOLINT
      EXPECT_EQ(stat_monitor_data.mean_frequency_avg_masked()[index_data_size], scalar_stats.mean_frequency_avg_masked[i_npol][i_ndim]); // NOLINT
      EXPECT_EQ(stat_monitor_data.variance_frequency_avg()[index_data_size], scalar_stats.variance_frequency_avg[i_npol][i_ndim]); // NOLINT
      EXPECT_EQ(stat_monitor_data.variance_frequency_avg_masked()[index_data_size], scalar_stats.variance_frequency_avg_masked[i_npol][i_ndim]); // NOLINT
      EXPECT_EQ(stat_monitor_data.num_clipped_samples()[index_data_size], scalar_stats.num_clipped_samples[i_npol][i_ndim]); // NOLINT
      EXPECT_EQ(stat_monitor_data.num_clipped_samples_masked()[index_data_size], scalar_stats.num_clipped_samples_masked[i_npol][i_ndim]); // NOLINT
      index_data_size++;
    }
  }

  // end scan
  usleep(ska::pst::common::microseconds_per_decisecond);
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - ending scan");
  handler->stop_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - scan ended");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - deconfiguring scan");
  handler->deconfigure_scan();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - scan deconfigured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - deconfiguring beam");
  handler->deconfigure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::controlled_get_monitor_data - beam deconfigured");
}

// TODO(jesmigel): Update at AT3-422
TEST_F(StatLmcServiceHandlerTest, get_env) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::get_env - beam deconfigured");
  ska::pst::lmc::GetEnvironmentResponse response;
  handler->get_env(&response);
  auto stats = _stat->get_scalar_stats();
}

TEST_F(StatLmcServiceHandlerTest, go_to_runtime_error) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::go_to_runtime_error");

  try
  {
    throw std::runtime_error("this is a test error");
  }
  catch (...)
  {
    handler->go_to_runtime_error(std::current_exception());
  }

  ASSERT_EQ(ska::pst::common::RuntimeError, _stat->get_state());
  ASSERT_EQ(ska::pst::common::RuntimeError, handler->get_application_manager_state());

  try
  {
    if (handler->get_application_manager_exception()) {
    std::rethrow_exception(handler->get_application_manager_exception());
    } else {
    // the exception should be set and not null
    ASSERT_FALSE(true);
    }
  }
  catch (const std::exception& exc)
  {
    ASSERT_EQ("this is a test error", std::string(exc.what()));
  }
}

TEST_F(StatLmcServiceHandlerTest, reset_non_runtime_error_state) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_non_runtime_error_state - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_non_runtime_error_state - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_non_runtime_error_state - configuring scan");
  EXPECT_FALSE(handler->is_scan_configured());
  configure_scan();
  EXPECT_TRUE(handler->is_scan_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_non_runtime_error_state - scan configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_non_runtime_error_state - resetting");
  handler->reset();
  EXPECT_TRUE(handler->is_scan_configured());
  EXPECT_TRUE(handler->is_beam_configured());

  EXPECT_EQ(ska::pst::common::State::ScanConfigured, handler->get_application_manager_state());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_non_runtime_error_state - reset");
}

TEST_F(StatLmcServiceHandlerTest, reset_from_runtime_error_state) // NOLINT
{
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_from_runtime_error_state - configuring beam");
  configure_beam();
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_from_runtime_error_state - beam configured");

  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_from_runtime_error_state - configuring scan");
  EXPECT_FALSE(handler->is_scan_configured());
  configure_scan();
  EXPECT_TRUE(handler->is_scan_configured());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_from_runtime_error_state - scan configured");

  try
  {
    throw std::runtime_error("force fault state");
  }
  catch (...)
  {
    handler->go_to_runtime_error(std::current_exception());
  }

  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_from_runtime_error_state - resetting");
  handler->reset();
  EXPECT_FALSE(handler->is_scan_configured());
  EXPECT_FALSE(handler->is_beam_configured());

  EXPECT_EQ(ska::pst::common::State::Idle, handler->get_application_manager_state());
  SPDLOG_TRACE("StatLmcServiceHandlerTest::reset_from_runtime_error_state - reset");
}

} // namespace ska::pst::stat::test
