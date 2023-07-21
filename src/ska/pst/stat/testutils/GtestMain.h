/*
 * Copyright 2022 Square Kilometre Array Observatory
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

#ifndef SKA_PST_STAT_TESTUTILS_GtestMain_h
#define SKA_PST_STAT_TESTUTILS_GtestMain_h

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include <gtest/gtest.h>
#pragma GCC diagnostic pop
#include <string>

namespace ska::pst::stat::test {

/*
 * @brief the data directory to find test data files
 */
std::string& test_data_dir();

/*
 * @brief return the filename with the test_data_dir prepended
 */
std::string test_data_file(std::string const& filename);

/**
 * @brief Get the number of shared memory bytes in used by the current process
 *
 * @return uint32_t number of shared memory bytes in use.
 */
uint32_t get_shared_memory_bytes_used();

/**
 * @brief
 *    Executable funtion to launch gtests
 */
int gtest_main(int argc, char** argv);

} // ska::pst::stat::test

#endif // SKA_PST_STAT_TESTUTILS_GtestMain_h

