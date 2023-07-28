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
#include <thread>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/Timer.h"
#include "ska/pst/stat/testutils/GtestMain.h"

#include "ska/pst/stat/tests/StatProcessorTest.h"


auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatProcessorTest::StatProcessorTest()
    : ::testing::Test()
{
}

void ska::pst::stat::test::StatProcessorTest::init_config()
{
  data_config.load_from_file(test_data_file("data_config.txt"));
  weights_config.load_from_file(test_data_file("weights_config.txt"));
}

void ska::pst::stat::test::StatProcessorTest::clear_config()
{
  data_config.reset();
  weights_config.reset();
  sp = nullptr;
}

void StatProcessorTest::SetUp()
{
  init_config();
}

void StatProcessorTest::TearDown()
{
  clear_config();
}

TEST_F(StatProcessorTest, test_construct_delete) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);
  SPDLOG_INFO("data_config:\n{}", sp->get_data_config().raw());
  ASSERT_EQ(sp->get_data_config().get_uint32("NPOL"), data_config.get_uint32("NPOL"));
  ASSERT_EQ(sp->get_data_config().get_uint32("NDIM"), data_config.get_uint32("NDIM"));
  ASSERT_EQ(sp->get_data_config().get_uint32("NCHAN"), data_config.get_uint32("NCHAN"));
  ASSERT_EQ(sp->get_data_config().get_uint32("NBIT"), data_config.get_uint32("NBIT"));
  ASSERT_EQ(sp->get_data_config().get_uint32("NPOL"), data_config.get_uint32("NPOL"));
  ASSERT_EQ(sp->get_data_config().get_uint32("UDP_NSAMP"), data_config.get_uint32("UDP_NSAMP"));
  ASSERT_EQ(sp->get_data_config().get_val("STAT_OUTPUT_BASEPATH"), data_config.get_val("STAT_OUTPUT_BASEPATH"));
  ASSERT_EQ(sp->get_data_config().get_uint32("STAT_REQ_TIME_BINS"), data_config.get_uint32("STAT_REQ_TIME_BINS"));
  ASSERT_EQ(sp->get_data_config().get_uint32("STAT_REQ_FREQ_BINS"), data_config.get_uint32("STAT_REQ_FREQ_BINS"));
  SPDLOG_INFO("weights_config:\n{}", sp->get_weights_config().raw());
  ASSERT_EQ(sp->get_weights_config().get_uint32("NPOL"), weights_config.get_uint32("NPOL"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NDIM"), weights_config.get_uint32("NDIM"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NCHAN"), weights_config.get_uint32("NCHAN"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NBIT"), weights_config.get_uint32("NBIT"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NPOL"), weights_config.get_uint32("NPOL"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("UDP_NSAMP"), weights_config.get_uint32("UDP_NSAMP"));
  ASSERT_EQ(sp->get_weights_config().get_val("STAT_OUTPUT_BASEPATH"), weights_config.get_val("STAT_OUTPUT_BASEPATH"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("STAT_REQ_TIME_BINS"), weights_config.get_uint32("STAT_REQ_TIME_BINS"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("STAT_REQ_FREQ_BINS"), weights_config.get_uint32("STAT_REQ_FREQ_BINS"));
}

TEST_F(StatProcessorTest, test_process_valid_values) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);
  size_t data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> data_block(data_length);

  size_t weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> weights_block(weights_length);

  ASSERT_NO_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length));
}

TEST_F(StatProcessorTest, test_constructor_threshold_overrides)
{
  data_config.set("STAT_REQ_TIME_BINS", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);
  size_t data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> data_block(data_length);

  size_t weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> weights_block(weights_length);

  ASSERT_NO_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length));
  
  init_config();
  data_config.set("STAT_REQ_FREQ_BINS", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  ASSERT_NO_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length));

}

TEST_F(StatProcessorTest, test_process_invalid_values) // NOLINT
{
  init_config();
  data_config.set("NCHAN", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  size_t data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> data_block(data_length);

  size_t weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> weights_block(weights_length);

  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length)
  );

  init_config();
  data_config.set("NBIT", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  data_block = std::vector<char>(data_length);

  weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  weights_block = std::vector<char>(weights_length);

  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length)
  );

  init_config();
  data_config.set("NPOL", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  data_block = std::vector<char>(data_length);

  weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  weights_block = std::vector<char>(weights_length);

  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length)
  );

  init_config();
  data_config.set("NDIM", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  data_block = std::vector<char>(data_length);

  weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  weights_block = std::vector<char>(weights_length);

  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    weights_length)
  );
}

TEST_F(StatProcessorTest, test_process_null_pointer_error) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  size_t data_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> data_block(data_length);

  size_t weights_length = sp->get_data_config().get_uint32("UDP_NSAMP") * sp->get_data_config().get_uint32("NCHAN") * sp->get_data_config().get_uint32("NDIM") * sp->get_data_config().get_uint32("NPOL") * (sp->get_data_config().get_uint32("NBIT")/8);
  std::vector<char> weights_block(weights_length);

  EXPECT_ANY_THROW(sp->process(
    nullptr,
    data_length,
    &weights_block[0],
    weights_length)
  );

  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    data_length,
    nullptr,
    weights_length)
  );
  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    0,
    &weights_block[0],
    weights_length)
  );

  EXPECT_ANY_THROW(sp->process(
    &data_block[0],
    data_length,
    &weights_block[0],
    0)
  );
}

} // namespace ska::pst::stat::test