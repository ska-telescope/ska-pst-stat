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
#include <memory>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/PacketGenerator.h"
#include "ska/pst/stat/StatComputer.h"
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/stat/tests/TestDataLayout.h"

#ifndef SKA_PST_STAT_TESTS_StatComputerTest_h
#define SKA_PST_STAT_TESTS_StatComputerTest_h

namespace ska::pst::stat::test {

/**
 * @brief Test the StatComputer class
 *
 * @details
 *
 */
class StatComputerTest : public ::testing::Test
{
  protected:
    void SetUp() override;

    void TearDown() override;

  public:
    StatComputerTest();

    ~StatComputerTest() = default;

    void configure(bool use_generator = true);

    void generate_packets(const uint32_t num_packets);

    //! returns a valid weights configuration that matches the valid data configuration
    ska::pst::common::AsciiHeader get_weights_config(const ska::pst::common::AsciiHeader& data_config);

  protected:
    ska::pst::common::AsciiHeader data_config;
    ska::pst::common::AsciiHeader weights_config;

    std::shared_ptr<ska::pst::stat::StatStorage> storage{nullptr};
    std::shared_ptr<TestDataLayout> layout{nullptr};
    std::shared_ptr<ska::pst::common::PacketGenerator> generator{nullptr};
    std::unique_ptr<ska::pst::stat::StatComputer> computer{nullptr};

    std::vector<char> data_buffer;
    std::vector<char> weights_buffer;

    //! number of polarisations represented in storage vectors
    uint32_t npol{2};

    //! number of dimensions represented in storage vectors
    uint32_t ndim{2};

    //! number of channels representated in storage vectors
    uint32_t nchan{0};

    //! Number of bits per sample in the data stream
    uint32_t nbit{0};

    //! Number of bytes per packet in the data stream
    uint32_t data_packet_stride{0};

    //! Number of bytes per packet in the weights stream
    uint32_t weights_packet_stride{0};

    //! Size of a complete heap of data in the data stream, in bytes
    uint32_t heap_resolution{0};

    //! Size of the complex packet of data in the data stream, in bytes
    uint32_t packet_resolution{0};

    //! Number of UDP packets per heap in the data stream
    uint32_t packets_per_heap{0};

};

} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatComputerTest_h
