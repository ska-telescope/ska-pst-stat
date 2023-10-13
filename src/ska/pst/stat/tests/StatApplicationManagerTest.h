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
#include "ska/pst/smrb/DataBlockCreate.h"
#include "ska/pst/smrb/DataBlockWrite.h"

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/testutils/DataBlockTestHelper.h"

#include "ska/pst/stat/StatApplicationManager.h"

#ifndef SKA_PST_STAT_TESTS_StatApplicationManagerTest_h
#define SKA_PST_STAT_TESTS_StatApplicationManagerTest_h

namespace ska::pst::stat::test {

/**
 * @brief Test the StatApplicationManagerTest class
 *
 * @details
 *
 */

class StatApplicationManagerTest : public ::testing::Test
{
  protected:
    void SetUp() override;

    void TearDown() override;

    ska::pst::common::AsciiHeader config;
    ska::pst::common::AsciiHeader header;
  public:
    StatApplicationManagerTest();

    ~StatApplicationManagerTest() = default;
    std::unique_ptr<DataBlockTestHelper> data_helper;
    std::unique_ptr<DataBlockTestHelper> weights_helper;

    void write_bytes_to_data_writer(uint64_t bytes_to_write);
    void write_bytes_to_weights_writer(uint64_t bytes_to_write);

    void setup_data_block();
    void setup_db_test_helper();
    void tear_down_data_block();

    ska::pst::common::AsciiHeader beam_config;
    ska::pst::common::AsciiHeader scan_config;
    ska::pst::common::AsciiHeader start_scan_config;

    ska::pst::common::AsciiHeader data_scan_config;
    ska::pst::common::AsciiHeader weights_scan_config;
    ska::pst::common::AsciiHeader data_header;
    ska::pst::common::AsciiHeader weights_header;

    std::unique_ptr<ska::pst::smrb::DataBlockCreate> _dbc_data{nullptr};
    std::unique_ptr<ska::pst::smrb::DataBlockWrite> _writer_data{nullptr};
    std::unique_ptr<ska::pst::smrb::DataBlockCreate> _dbc_weights{nullptr};
    std::unique_ptr<ska::pst::smrb::DataBlockWrite> _writer_weights{nullptr};

    uint64_t data_bufsz{0};
    uint64_t weights_bufsz{0};
    std::string data_key;
    std::string weights_key;

    std::vector<char> data_to_write;
    std::vector<char> weights_to_write;

    std::unique_ptr<ska::pst::stat::StatApplicationManager> sm{nullptr};
    std::string stat_base_path{"/tmp"};

};
} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatApplicationManagerTest_h
