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

#include "ska/pst/smrb/DataBlockCreate.h"
#include "ska/pst/smrb/DataBlockWrite.h"
#include "ska/pst/smrb/DataBlockRead.h"
#include "ska/pst/common/utils/AsciiHeader.h"

#include <memory>

#ifndef SKA_PST_STAT_TESTUTILS_DataBlockTestHelper_h
#define SKA_PST_STAT_TESTUTILS_DataBlockTestHelper_h

namespace ska::pst::stat::test {

/**
 * @brief Helper for testing classes that read from a ring buffer in shared memory
 *
 * @details This class creates a ring buffer in shared memory, writes buffers to it, and optionally reads/clears buffers from it
 *
 */
class DataBlockTestHelper
{
  public:

    DataBlockTestHelper(std::string id = "dada", unsigned num_readers = 1);

    void set_header_block_nbufs(uint64_t hdr_nbufs);
    void set_header_block_bufsz(uint64_t hdr_bufsz);

    void set_data_block_nbufs(uint64_t hdr_nbufs);
    void set_data_block_bufsz(uint64_t hdr_bufsz);

    ~DataBlockTestHelper() = default;

    void setup();
    void enable_reader();
    void teardown();

    void set_config(const ska::pst::common::AsciiHeader&);  // configure scan
    void set_header(const ska::pst::common::AsciiHeader&);  // start of scan (with UTC_START)

    void start();

    void write(size_t nblocks, float delay_ms = 0.0);
    void write_and_close(size_t nblocks, float delay_ms);

    ska::pst::common::AsciiHeader config;
    ska::pst::common::AsciiHeader header;

  protected:

    std::string id{"dada"};
    std::shared_ptr<ska::pst::smrb::DataBlockCreate> db{nullptr};
    std::shared_ptr<ska::pst::smrb::DataBlockWrite> writer{nullptr}; // writes to ring buffer
    std::shared_ptr<ska::pst::smrb::DataBlockRead> reader{nullptr};  // clears buffers for more writing
    int device_id{-1};

    uint64_t hdr_nbufs{5};
    uint64_t hdr_bufsz{8192};
    uint64_t dat_nbufs{6};
    uint64_t dat_bufsz{1048576};
    unsigned num_readers{1};

    uint64_t counter{0};
};

} // ska::pst::stat::test

#endif // SKA_PST_SMRB_TESTUTILS_DataBlockTestHelper_h

