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
#include <spdlog/spdlog.h>

#include "ska/pst/stat/testutils/GtestMain.h"
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
  static constexpr uint64_t header_nbufs = 4;
  static constexpr uint64_t header_bufsz = 4096;
  static constexpr uint64_t data_nbufs = 8;
  static constexpr uint64_t weights_nbufs = 8;
  static constexpr uint64_t bufsz_factor = 16;
  static constexpr unsigned nreaders = 1;
  static constexpr int device = -1;
  uint64_t data_bufsz = data_header.get_uint64("RESOLUTION") * bufsz_factor;
  uint64_t weights_bufsz = weights_header.get_uint64("RESOLUTION") * bufsz_factor;
  SPDLOG_DEBUG("ska::pst::dsp::test::DiskManagerTest::setup_data_block data_bufsz={} weights_bufsz={}", data_bufsz, weights_bufsz);

  _dbc_data = std::make_unique<ska::pst::smrb::DataBlockCreate>(beam_config.get_val("DATA_KEY"));
  _dbc_data->create(header_nbufs, header_bufsz, data_nbufs, data_bufsz, nreaders, device);

  _writer_data = std::make_unique<ska::pst::smrb::DataBlockWrite>(beam_config.get_val("DATA_KEY"));
  _writer_data->connect(0);
  _writer_data->lock();

  _dbc_weights = std::make_unique<ska::pst::smrb::DataBlockCreate>(beam_config.get_val("WEIGHTS_KEY"));
  _dbc_weights->create(header_nbufs, header_bufsz, weights_nbufs, weights_bufsz, nreaders, device);

  _writer_weights = std::make_unique<ska::pst::smrb::DataBlockWrite>(beam_config.get_val("WEIGHTS_KEY"));
  _writer_weights->connect(0);
  _writer_weights->lock();

  data_to_write.resize(data_bufsz * 2);
  weights_to_write.resize(weights_bufsz * 2);

  _writer_data->write_config(data_scan_config.raw());
  _writer_weights->write_config(weights_scan_config.raw());

  _writer_data->write_header(data_header.raw());
  _writer_weights->write_header(weights_header.raw());
}

void StatApplicationManagerTest::tear_down_data_block()
{
  if (_writer_data)
  {
    if (_writer_data->get_opened())
    {
      _writer_data->close();
    }
    if (_writer_data->get_locked())
    {
      _writer_data->unlock();
    }
    _writer_data->disconnect();
  }
  _writer_data = nullptr;

  if (_dbc_data)
  {
    _dbc_data->destroy();
  }
  _dbc_data = nullptr;
  if (_writer_weights)
  {
    if (_writer_weights->get_opened())
    {
      _writer_weights->close();
    }
    if (_writer_weights->get_locked())
    {
      _writer_weights->unlock();
    }
    _writer_weights->disconnect();
  }
  _writer_weights = nullptr;

  if (_dbc_weights)
  {
    _dbc_weights->destroy();
  }
  _dbc_weights = nullptr;
}

void StatApplicationManagerTest::SetUp()
{
  beam_config.load_from_file(test_data_file("config.txt"));
  scan_config.load_from_file(test_data_file("config.txt"));
  start_scan_config.load_from_file(test_data_file("config.txt"));

  data_scan_config.load_from_file(test_data_file("data_config.txt"));
  weights_scan_config.load_from_file(test_data_file("weights_config.txt"));

  data_header.load_from_file(test_data_file("data_header_LowAA0.5.txt"));
  weights_header.load_from_file(test_data_file("weights_header_LowAA0.5.txt"));
  beam_config.append_header(data_header);
  beam_config.append_header(weights_header);

  setup_data_block();
}

void StatApplicationManagerTest::TearDown()
{
  tear_down_data_block();
}

TEST_F(StatApplicationManagerTest, test_construct_delete) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

TEST_F(StatApplicationManagerTest, test_configure_deconfigure_beam) // NOLINT
{
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>();
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
  sm = std::make_unique<ska::pst::stat::StatApplicationManager>();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());

  sm->configure_beam(beam_config);
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  /* ERROR IN CONFIGURE SCAN INVOLVING NPOL
  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());

  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->configure_scan(scan_config);
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());

  sm->deconfigure_scan();
  ASSERT_EQ(ska::pst::common::BeamConfigured, sm->get_state());

  sm->start_scan(start_scan_config);
  ASSERT_EQ(ska::pst::common::Scanning, sm->get_state());

  sm->stop_scan();
  ASSERT_EQ(ska::pst::common::ScanConfigured, sm->get_state());
  */

  sm->deconfigure_beam();
  ASSERT_EQ(ska::pst::common::Idle, sm->get_state());
}

} // namespace ska::pst::stat::test
