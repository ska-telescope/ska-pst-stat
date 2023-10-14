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

#include <memory>
#include <grpc++/grpc++.h>
#include <gtest/gtest.h>
#include "ska/pst/smrb/DataBlockCreate.h"
#include "ska/pst/smrb/DataBlockWrite.h"

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/testutils/DataBlockTestHelper.h"

#include "ska/pst/stat/lmc/StatLmcServiceHandler.h"

#include "ska/pst/stat/StatApplicationManager.h"

#ifndef SKA_PST_STAT_TESTS_StatLmcServiceHandlerTest_h
#define SKA_PST_STAT_TESTS_StatLmcServiceHandlerTest_h

namespace ska {
namespace pst {
namespace stat {
namespace test {

/**
 * @brief Unit testing the handler against a Stat Application Manager Manager.
 *
 * @details
 *
 */
class StatLmcServiceHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override;
    void TearDown() override;



  public:
    StatLmcServiceHandlerTest();
    ~StatLmcServiceHandlerTest() = default;

    // void setup_reader_writers();

    // helper methods for common repeated code.
    void setup_data_block();
    void tear_down_data_block();

    void configure_beam();
    void configure_scan();
    void start_scan();

    uint64_t data_bufsz;
    uint64_t weights_bufsz;
    std::string data_key;
    std::string weights_key;
    ska::pst::common::AsciiHeader beam_config;
    ska::pst::common::AsciiHeader scan_config;
    ska::pst::common::AsciiHeader start_scan_config;

    ska::pst::common::AsciiHeader data_scan_config;
    ska::pst::common::AsciiHeader weights_scan_config;

    ska::pst::common::AsciiHeader data_header;
    ska::pst::common::AsciiHeader weights_header;

    std::unique_ptr<DataBlockTestHelper> data_helper;
    std::unique_ptr<DataBlockTestHelper> weights_helper;

    std::shared_ptr<ska::pst::stat::StatLmcServiceHandler> handler{nullptr};
    std::shared_ptr<ska::pst::stat::StatApplicationManager> _stat{nullptr};

};

}
}
}
} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatLmcServiceHandlerTest_h
