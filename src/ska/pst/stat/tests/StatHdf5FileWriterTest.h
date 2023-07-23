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
#include "ska/pst/stat/StatHdf5FileWriter.h"

#ifndef SKA_PST_DSP_TESTS_StatHdf5FileWriterTest_h
#define SKA_PST_DSP_TESTS_StatHdf5FileWriterTest_h

namespace ska {
namespace pst {
namespace stat {
namespace test {

struct DatasetElement {
    int EXECUTION_BLOCK_ID;
    int SCAN_ID;
    int BEAM_ID;
    double T_MIN_MJD;
    double T_MAX_MJD;
    double TIME_OFFSET_SECONDS;
    int NDAT;
    double FREQ_MHZ;
    int START_CHAN;
    double BW_MHZ;
    int NPOL;
    int NDIM;
    int NCHAN_input;
    int NCHAN_DS;
    int NDAT_DS;
    int NBIN_HIST;
    int NBIN_HIST2D;
    double CHAN_FREQ_MHZ;
};

/**
 * @brief Test the StatHdf5FileWriter class
 *
 * @details
 *
 */
class StatHdf5FileWriterTest : public ::testing::Test
{
    protected:
      void SetUp() override;

      void TearDown() override;

    public:
        StatHdf5FileWriterTest();

        ~StatHdf5FileWriterTest() = default;

    private:

};

} // namespace test
} // namespace stat
} // namespace pst
} // namespace ska

#endif // SKA_PST_SMRB_TESTS_StatHdf5FileWriterTest_h
