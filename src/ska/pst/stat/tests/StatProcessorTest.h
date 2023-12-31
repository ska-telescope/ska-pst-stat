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
#include "ska/pst/common/definitions.h"
#include "ska/pst/stat/StatProcessor.h"

#ifndef SKA_PST_STAT_TESTS_StatProcessorTest_h
#define SKA_PST_STAT_TESTS_StatProcessorTest_h

namespace ska::pst::stat::test {

  class TestStatProcessor : public StatProcessor
  {
    public:
      TestStatProcessor(const ska::pst::common::AsciiHeader data_config, const ska::pst::common::AsciiHeader weights_config) :
      StatProcessor(data_config, weights_config)
      {
        this->data_config=data_config;this->weights_config=weights_config;
      };

      ~TestStatProcessor() = default;

      ska::pst::common::AsciiHeader get_data_config() { return data_config; }
      ska::pst::common::AsciiHeader get_weights_config() { return weights_config; }
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

      ska::pst::common::AsciiHeader data_config;
      ska::pst::common::AsciiHeader weights_config;

      std::shared_ptr<TestStatProcessor> sp;
      size_t get_data_length() {
        return sp->get_data_config().get_uint32("RESOLUTION");
      };
      size_t get_weights_length() {
        return sp->get_weights_config().get_uint32("RESOLUTION");
      };

      size_t get_data_packet_size() {
        auto npol = sp->get_data_config().get_uint32("NPOL");
        auto ndim = sp->get_data_config().get_uint32("NDIM");
        auto nbit = sp->get_data_config().get_uint32("NBIT");
        auto nsamp = sp->get_data_config().get_uint32("UDP_NSAMP");
        auto nchan_per_packet = sp->get_data_config().get_uint32("UDP_NCHAN");

        return static_cast<size_t>(npol * ndim * nbit * nsamp * nchan_per_packet / ska::pst::common::bits_per_byte);
      }

      size_t get_weights_packet_size() {
        auto npol = sp->get_weights_config().get_uint32("NPOL");
        auto ndim = sp->get_weights_config().get_uint32("NDIM");
        auto packet_scale_size = sp->get_weights_config().get_uint32("PACKET_SCALES_SIZE");
        auto packet_weights_size = sp->get_weights_config().get_uint32("PACKET_WEIGHTS_SIZE");

        return static_cast<size_t>(npol * ndim * (packet_scale_size + packet_weights_size));
      }

      size_t get_packets_per_resolution() {
        auto resolution = get_data_length();
        auto packet_size = get_data_packet_size();

        return resolution / packet_size;
      }

      void init_segment();

      std::vector<char> data_block;
      std::vector<char> weights_block;

      ska::pst::common::SegmentProducer::Segment segment;

    private:

  };
} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatProcessorTest_h
