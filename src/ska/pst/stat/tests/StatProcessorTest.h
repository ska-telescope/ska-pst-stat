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


#include <gtest/gtest.h>
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatProcessor.h"

#ifndef SKA_PST_STAT_TESTS_StatProcessorTest_h
#define SKA_PST_STAT_TESTS_StatProcessorTest_h

namespace ska::pst::stat::test {
/**
  */
  class TestStatProcessor : public StatProcessor
  {
    public:
      TestStatProcessor(const ska::pst::common::AsciiHeader config) : StatProcessor(config) {this->config=config;};
  
      ~TestStatProcessor() = default;

      ska::pst::common::AsciiHeader get_config() { return config; }
      std::shared_ptr<StatStorage> get_storage() { return storage; }
  };

/**
 * @brief Test the StatProcessor class
 *
 * @details
 *
 */
  class StatProcessorTest : public ::testing::Test
  {
    protected:
      void SetUp() override;
  
      void TearDown() override;
  
    public:
      StatProcessorTest();
  
      ~StatProcessorTest() = default;

      void init_config();
      void clear_config();
  
      ska::pst::common::AsciiHeader config;
  
      std::shared_ptr<TestStatProcessor> sp;
  
    private:
  
  };
} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatProcessorTest_h