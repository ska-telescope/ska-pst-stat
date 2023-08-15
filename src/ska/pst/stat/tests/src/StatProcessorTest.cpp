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

void ska::pst::stat::test::StatProcessorTest::init_segment ()
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  data_block.resize(get_data_length());
  weights_block.resize(get_weights_length());

  segment.data.block = data_block.data();
  segment.data.size = data_block.size();
  segment.weights.block = weights_block.data();
  segment.weights.size = weights_block.size();
}

void StatProcessorTest::SetUp()
{
  init_config();
  init_segment();
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
  ASSERT_EQ(sp->get_data_config().get_val("STAT_OUTPUT_FILENAME"), data_config.get_val("STAT_OUTPUT_FILENAME"));
  ASSERT_EQ(sp->get_data_config().get_uint32("STAT_REQ_TIME_BINS"), data_config.get_uint32("STAT_REQ_TIME_BINS"));
  ASSERT_EQ(sp->get_data_config().get_uint32("STAT_REQ_FREQ_BINS"), data_config.get_uint32("STAT_REQ_FREQ_BINS"));
  SPDLOG_INFO("weights_config:\n{}", sp->get_weights_config().raw());
  ASSERT_EQ(sp->get_weights_config().get_uint32("NPOL"), weights_config.get_uint32("NPOL"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NDIM"), weights_config.get_uint32("NDIM"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NCHAN"), weights_config.get_uint32("NCHAN"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NBIT"), weights_config.get_uint32("NBIT"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("NPOL"), weights_config.get_uint32("NPOL"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("UDP_NSAMP"), weights_config.get_uint32("UDP_NSAMP"));
  ASSERT_EQ(sp->get_weights_config().get_val("STAT_OUTPUT_FILENAME"), weights_config.get_val("STAT_OUTPUT_FILENAME"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("STAT_REQ_TIME_BINS"), weights_config.get_uint32("STAT_REQ_TIME_BINS"));
  ASSERT_EQ(sp->get_weights_config().get_uint32("STAT_REQ_FREQ_BINS"), weights_config.get_uint32("STAT_REQ_FREQ_BINS"));
}

TEST_F(StatProcessorTest, test_process_valid_values) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);
  ASSERT_NO_THROW(sp->process(segment));
}

TEST_F(StatProcessorTest, test_constructor_threshold_overrides) // NOLINT
{
  data_config.set("STAT_REQ_TIME_BINS", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  ASSERT_NO_THROW(sp->process(segment));

  init_config();
  data_config.set("STAT_REQ_FREQ_BINS", 0);
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  ASSERT_NO_THROW(sp->process(segment));
}

TEST_F(StatProcessorTest, test_process_length_of_zero) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  size_t data_length = 0;
  std::vector<char> data_block(data_length);

  size_t weights_length = 0;
  std::vector<char> weights_block(weights_length);

  ska::pst::common::SegmentProducer::Segment bad_segment;
  bad_segment.data.block = data_block.data();
  bad_segment.data.size = data_length;
  bad_segment.weights.block = weights_block.data();
  bad_segment.weights.size = weights_length;

  EXPECT_ANY_THROW(sp->process(bad_segment));
}

TEST_F(StatProcessorTest, test_process_null_pointer_error) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  ska::pst::common::SegmentProducer::Segment bad_segment;

  bad_segment = segment;
  bad_segment.data.block = nullptr;
  EXPECT_ANY_THROW(sp->process(bad_segment));

  bad_segment = segment;
  bad_segment.weights.block = nullptr;
  EXPECT_ANY_THROW(sp->process(bad_segment));

  bad_segment = segment;
  bad_segment.data.size = 0;
  EXPECT_ANY_THROW(sp->process(bad_segment));

  bad_segment = segment;
  bad_segment.weights.size = 0;
  EXPECT_ANY_THROW(sp->process(bad_segment));
}

TEST_F(StatProcessorTest, test_length_multiple_of_resolution) // NOLINT
{
  sp = std::make_shared<TestStatProcessor>(data_config, weights_config);

  ska::pst::common::SegmentProducer::Segment bad_segment;

  bad_segment = segment;
  bad_segment.data.size = 3;
  EXPECT_ANY_THROW(sp->process(bad_segment));

  bad_segment = segment;
  bad_segment.weights.size = 3;
  EXPECT_ANY_THROW(sp->process(bad_segment));
}

TEST_F(StatProcessorTest, test_resolution_multiple_of_bytes_per_sample) // NOLINT
{
  init_config();
  data_config.set("RESOLUTION", 3);
  EXPECT_ANY_THROW(sp = std::make_shared<TestStatProcessor>(data_config, weights_config));
}

} // namespace ska::pst::stat::test
