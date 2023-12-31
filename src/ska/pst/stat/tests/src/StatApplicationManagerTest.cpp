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

#include <cmath>
#include <vector>
#include <regex>
#include <filesystem>
#include <spdlog/spdlog.h>

#include "ska/pst/stat/testutils/GtestMain.h"
#include "ska/pst/smrb/DataBlockRead.h"
#include "ska/pst/common/utils/FileWriter.h"

#include "ska/pst/stat/StatFilenameConstructor.h"
#include "ska/pst/stat/tests/StatApplicationManagerTest.h"

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatApplicationManagerTest::StatApplicationManagerTest()
    : ::testing::Test()
{
}


void StatApplicationManagerTest::setup_data_block()
{
  static uint64_t header_nbufs = beam_config.get_uint64("HB_NBUFS");
  static uint64_t header_bufsz = beam_config.get_uint64("HB_BUFSZ");
  static uint64_t data_nbufs = beam_config.get_uint64("DB_NBUFS");
  static uint64_t weights_nbufs = beam_config.get_uint64("WB_NBUFS");

  static constexpr uint64_t bufsz_factor = 16;
  static constexpr unsigned nreaders = 1;
  static constexpr int device = -1;
  data_bufsz = data_header.get_uint64("RESOLUTION") * bufsz_factor;
  weights_bufsz = weights_header.get_uint64("RESOLUTION") * bufsz_factor;

  data_helper = std::make_unique<DataBlockTestHelper>(beam_config.get_val("DATA_KEY"), 1);
  weights_helper = std::make_unique<DataBlockTestHelper>(beam_config.get_val("WEIGHTS_KEY"), 1);

  weights_helper->set_data_block_bufsz(weights_bufsz);
  data_helper->set_data_block_bufsz(data_bufsz);

  weights_helper->set_data_block_nbufs(weights_nbufs);
  data_helper->set_data_block_nbufs(data_nbufs);

  weights_helper->set_header_block_nbufs(header_nbufs);
  data_helper->set_header_block_nbufs(header_nbufs);

  weights_helper->set_header_block_bufsz(header_bufsz);
  data_helper->set_header_block_bufsz(header_bufsz);

  weights_helper->set_config(weights_header);
  data_helper->set_config(data_header);

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

void StatApplicationManagerTest::tear_down_data_block()
{
  SPDLOG_TRACE("tear_down_data_block teardown");
  data_helper->teardown();
  weights_helper->teardown();
  SPDLOG_TRACE("tear_down_data_block teardown complete");
}

void StatApplicationManagerTest::SetUp()
{
  beam_config.load_from_file(test_data_file("beam_config.txt"));
  scan_config.load_from_file(test_data_file("scan_config.txt"));
  start_scan_config.load_from_file(test_data_file("start_scan_config.txt"));

  data_header.load_from_file(test_data_file("data_header_LowAA0.5.txt"));
  weights_header.load_from_file(test_data_file("weights_header_LowAA0.5.txt"));

  setup_data_block();
}

void StatApplicationManagerTest::TearDown()
{
  tear_down_data_block();
  sm = nullptr;
}

TEST_F(StatApplicationManagerTest, test_construct_delete) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_configure_deconfigure_beam) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_multiple_configure_deconfigure_beam) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_configure_deconfigure_scan) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());

  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_multiple_configure_deconfigure_scan) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());

  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());

  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_start_stop_scan) // NOLINT
{
  SPDLOG_TRACE("test_start_stop_scan create StatApplicationManager");
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  SPDLOG_TRACE("test_start_stop_scan sm->configure_beam");
  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->configure_beam complete");

  SPDLOG_TRACE("test_start_stop_scan sm->configure_scan");
  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->configure_scan complete");

  SPDLOG_TRACE("test_start_stop_scan sm->start_scan");
  sm->start_scan(start_scan_config);
  ASSERT_EQ(ska::pst::common::Scanning, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->start_scan complete");

  static constexpr float delay_ms = 1000;
  size_t constexpr test_nblocks = 4;
  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  SPDLOG_TRACE("test_start_stop_scan sm->stop_scan");
  sm->stop_scan();
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->stop_scan complete");

  SPDLOG_TRACE("test_start_stop_scan sm->deconfigure_scan");
  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->deconfigure_scan complete");
  SPDLOG_TRACE("test_start_stop_scan sm->deconfigure_beam");
  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->deconfigure_beam complete");
}

TEST_F(StatApplicationManagerTest, test_configure_from_file) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_from_file(test_data_file("config.txt"));
  ASSERT_EQ(ska::pst::common::Scanning, sm->get_state());
  SPDLOG_TRACE("test_start_stop_scan sm->start_scan complete");

  static constexpr float delay_ms = 1000;
  size_t constexpr test_nblocks = 4;
  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  SPDLOG_DEBUG("Busy waiting for the processing to be complete");
  static constexpr uint64_t max_to_wait = 60 * ska::pst::common::microseconds_per_second;
  unsigned waited_so_far = 0;
  while (sm->get_processing_state() == ska::pst::stat::StatApplicationManager::ProcessingState::Processing && waited_so_far < max_to_wait)
  {
    usleep(ska::pst::common::microseconds_per_decisecond);
    waited_so_far += ska::pst::common::microseconds_per_decisecond;
  }

  sleep(1);

  StatStorage::scalar_stats_t ss = sm->get_scalar_stats();
  for (unsigned ipol=0; ipol<ss.mean_frequency_avg.size(); ipol++) // NOLINT
  {
    for (unsigned idim=0; idim<ss.mean_frequency_avg[ipol].size(); idim++) // NOLINT
    {
      ASSERT_EQ(ss.mean_frequency_avg[ipol][idim], 0);
      ASSERT_EQ(ss.variance_frequency_avg[ipol][idim], 0);
      ASSERT_EQ(ss.num_clipped_samples[ipol][idim], 0);
    }
  }

  data_header.set_val("STAT_BASE_PATH", stat_base_path);
  ska::pst::stat::StatFilenameConstructor namer(data_header);
  uint64_t file_number = 0;
  std::filesystem::path stat_file = namer.get_filename(data_header.get_val("UTC_START"), data_header.get_uint64("OBS_OFFSET"), file_number);
  SPDLOG_DEBUG("test_configure_from_file expected stat_file={}", stat_file.generic_string());

  std::string path = stat_file.parent_path();
  std::ostringstream oss;
  oss << path << "/" << data_header.get_val("UTC_START") << "_(.*)_00000(.*).h5";
  std::string regex_pattern = oss.str();
  for (const auto & entry : std::filesystem::directory_iterator(path))
  {
    SPDLOG_DEBUG("Assessing file {}", entry.path().generic_string());
    ASSERT_TRUE(std::regex_match(entry.path().generic_string(), std::regex(regex_pattern)));
  }

  sm->stop_scan();
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());

  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->quit();
  ASSERT_EQ(ska::pst::common::Unknown, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_force_runtime_error) // NOLINT
{
  SPDLOG_TRACE("test_force_runtime_error create StatApplicationManager");
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>(stat_base_path);
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  SPDLOG_TRACE("test_force_runtime_error sm->configure_beam");
  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());
  SPDLOG_TRACE("test_force_runtime_error sm->configure_beam complete");

  SPDLOG_TRACE("test_force_runtime_error sm->configure_scan");
  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());
  SPDLOG_TRACE("test_force_runtime_error sm->configure_scan complete");

  // set the SCAN_ID to something different
  start_scan_config.set("SCAN_ID", data_header.get_uint64("SCAN_ID") + 1);

  SPDLOG_TRACE("test_force_runtime_error sm->start_scan");
  sm->start_scan(start_scan_config);
  ASSERT_EQ(ska::pst::common::Scanning, sm->get_state());
  SPDLOG_TRACE("test_force_runtime_error sm->start_scan complete");

  static constexpr float delay_ms = 1000;
  size_t constexpr test_nblocks = 4;
  std::thread data_thread = std::thread(&DataBlockTestHelper::write_and_close, data_helper.get(), test_nblocks, delay_ms);
  std::thread weights_thread = std::thread(&DataBlockTestHelper::write_and_close, weights_helper.get(), test_nblocks, delay_ms);
  data_thread.join();
  weights_thread.join();

  SPDLOG_TRACE("test_force_runtime_error asserting state is RuntimeError");
  ASSERT_EQ(ska::pst::common::RuntimeError, sm->get_state());

  SPDLOG_TRACE("test_force_runtime_error sm->reset()");
  sm->reset();

  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
  SPDLOG_TRACE("test_force_runtime_error sm->reset complete");
}

} // namespace ska::pst::stat::test
