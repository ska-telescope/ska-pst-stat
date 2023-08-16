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

#include "ska/pst/stat/tests/FileProcessorTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"

#include <spdlog/spdlog.h>
#include <filesystem>

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test
{

FileProcessorTest::FileProcessorTest() : ::testing::Test()
{
}

void FileProcessorTest::SetUp()
{
}

void FileProcessorTest::TearDown()
{
}

TEST_F(FileProcessorTest, test_output_filename) // NOLINT
{
  ASSERT_EQ(processor.get_output_filename("anything.ext"), "anything.h5");

  // get_output_filename creates the stat/ sub-folder in the current working directory
  ASSERT_EQ(processor.get_output_filename("folder/anything.ext"), "stat/anything.h5");
  ASSERT_EQ(std::filesystem::is_directory("stat"), true);
  std::filesystem::remove("stat");

  // get_output_filename creates the stat/ sub-folder in the root directory of the data file
  ASSERT_EQ(processor.get_output_filename("/tmp/subfolder/anything.ext"), "/tmp/stat/anything.h5");
  ASSERT_EQ(std::filesystem::is_directory("/tmp/stat"), true);
  std::filesystem::remove("/tmp/stat");
}

TEST_F(FileProcessorTest, test_match) // NOLINT
{
  ska::pst::common::AsciiHeader data_config;
  ska::pst::common::AsciiHeader weights_config;

  data_config.load_from_file(test_data_file("data_config.txt"));
  weights_config.load_from_file(test_data_file("weights_config.txt"));

  ska::pst::common::HeapLayout layout;
  layout.configure(data_config, weights_config);

  uint64_t delta_heap = 1024; // NOLINT
  unsigned ntest = 1024; // NOLINT
  uint64_t heap_offset = 0;

  for(unsigned i=0; i<ntest; i++)
  {
    auto data_byte_offset = heap_offset * layout.get_data_heap_stride();
    auto weights_byte_offset = heap_offset * layout.get_weights_heap_stride();

    data_config.set("OBS_OFFSET", data_byte_offset);
    weights_config.set("OBS_OFFSET", weights_byte_offset);

    // should not throw exception
    processor.match(data_config, weights_config);

    heap_offset += delta_heap;
  }

  // offset the weights OBS_OFFSET from the data OBS_OFFSET
  auto weights_byte_offset = heap_offset * layout.get_weights_heap_stride();
  weights_config.set("OBS_OFFSET", weights_byte_offset);
  EXPECT_THROW(processor.match(data_config, weights_config),std::runtime_error);
}

} // namespace ska::pst::stat::test
