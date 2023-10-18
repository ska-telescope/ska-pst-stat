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

#include <sstream>
#include <filesystem>
#include <spdlog/spdlog.h>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/FileWriter.h"
#include "ska/pst/stat/tests/StatFilenameConstructorTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"

#include <string>

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatFilenameConstructorTest::StatFilenameConstructorTest()
    : ::testing::Test()
{
}

void StatFilenameConstructorTest::SetUp()
{
  header.load_from_file(test_data_file("data_header_LowAA0.5.txt"));
  header.set_val("STAT_BASE_PATH", "/tmp");

  utc_start = header.get_val("UTC_START");
  obs_offset = header.get_uint64("OBS_OFFSET");
  file_number = 0;

  std::ostringstream oss;
  std::filesystem::path dada_filename = ska::pst::common::FileWriter::get_filename(utc_start, obs_offset, file_number);
  oss << header.get_val("STAT_BASE_PATH") << "/product/" << header.get_val("EB_ID") << "/"
      << StatFilenameConstructor::get_subsystem_from_telescope(header.get_val("TELESCOPE"))  << "/"
      << header.get_val("SCAN_ID") << "/monitoring_stats/" << dada_filename.replace_extension("h5").generic_string();
  expected_filename = oss.str();
}

void StatFilenameConstructorTest::TearDown()
{
}

TEST_F(StatFilenameConstructorTest, default_constructor) // NOLINT
{
  StatFilenameConstructor namer;

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_base_path(header.get_val("STAT_BASE_PATH"));

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_eb_id(header.get_val("EB_ID"));

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_telescope(header.get_val("TELESCOPE"));

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_scan_id(header.get_val("SCAN_ID"));

  std::filesystem::path filename = namer.get_filename(utc_start, obs_offset, file_number);
  ASSERT_EQ(filename.generic_string(), expected_filename);
};

TEST_F(StatFilenameConstructorTest, header_constructor) // NOLINT
{
  StatFilenameConstructor namer(header);

  std::filesystem::path filename = namer.get_filename(utc_start, obs_offset, file_number);
  ASSERT_EQ(filename.generic_string(), expected_filename);
};

TEST_F(StatFilenameConstructorTest, empty_header_constructor) // NOLINT
{
  ska::pst::common::AsciiHeader empty_header;
  StatFilenameConstructor namer(empty_header);

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_base_path(header.get_val("STAT_BASE_PATH"));

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_eb_id(header.get_val("EB_ID"));

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_telescope(header.get_val("TELESCOPE"));

  EXPECT_THROW(namer.get_filename(utc_start, obs_offset, file_number), std::runtime_error); // NOLINT
  namer.set_scan_id(header.get_val("SCAN_ID"));

  std::filesystem::path filename = namer.get_filename(utc_start, obs_offset, file_number);
  ASSERT_EQ(filename.generic_string(), expected_filename);
};

TEST_F(StatFilenameConstructorTest, bad_telescope_name) // NOLINT
{
  EXPECT_THROW(StatFilenameConstructor::get_subsystem_from_telescope("BadNameOfTelescope"), std::runtime_error);
  EXPECT_EQ(StatFilenameConstructor::get_subsystem_from_telescope("SKALow"), "pst-low");
  EXPECT_EQ(StatFilenameConstructor::get_subsystem_from_telescope("SKAMid"), "pst-mid");
}
} // namespace ska::pst::stat::test
