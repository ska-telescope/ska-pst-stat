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
#include <vector>

#include "ska/pst/common/definitions.h"
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/DataGenerator.h"

#ifndef SKA_PST_STAT_TESTS_TestDataLayout_h
#define SKA_PST_STAT_TESTS_TestDataLayout_h

namespace ska::pst::stat::test {

  class TestDataLayout : public ska::pst::common::PacketLayout
  {
    public:
    TestDataLayout (const ska::pst::common::AsciiHeader& config)
    {
      uint32_t ndim = config.get_uint32("NDIM");
      uint32_t npol = config.get_uint32("NPOL");
      uint32_t nbit = config.get_uint32("NBIT");

      nsamp_per_packet = config.get_uint32("UDP_NSAMP");
      nchan_per_packet = config.get_uint32("UDP_NCHAN");

      // test will have 2 buffers (scales+weights) and packet data
      packet_scales_size = config.get_uint32("PACKET_SCALES_SIZE"); // NOLINT
      packet_scales_offset = 0;

      packet_weights_size = config.get_uint32("PACKET_WEIGHTS_SIZE");
      packet_weights_offset = packet_scales_size;

      packet_data_size = nsamp_per_packet * nchan_per_packet * ndim * npol * nbit / ska::pst::common::bits_per_byte;
      packet_data_offset = 0;

      packet_size = packet_data_size;
    }
  };

} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_TestDataLayout_h
