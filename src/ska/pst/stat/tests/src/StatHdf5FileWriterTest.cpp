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

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/Timer.h"
#include "ska/pst/stat/tests/StatHdf5FileWriterTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"

#include <iostream>
#include <string>
#include <vector>
#include <H5Cpp.h>

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatHdf5FileWriterTest::StatHdf5FileWriterTest()
    : ::testing::Test()
{
}

void StatHdf5FileWriterTest::SetUp()
{
}

void StatHdf5FileWriterTest::TearDown()
{
}

TEST_F(StatHdf5FileWriterTest, test_construct_delete) // NOLINT
{
  // std::shared_ptr<StatHdf5FileWriter> sfw = std::make_shared<StatHdf5FileWriter>();
}

TEST_F(StatHdf5FileWriterTest, test_hdf5_api) // NOLINT
{
  // Initialize the HDF5 library
  // Suppress the printing of exceptions, making the output cleaner in case of an error.
  H5::Exception::dontPrint();

  // Create an HDF5 file
  H5::H5File file("/tmp/sample_dataset.h5", H5F_ACC_TRUNC);

  // Define the dataset dimensions
  const int rank = 1;
  std::array<hsize_t, rank> dims = {1};

  // Create a compound datatype for the schema
  H5::CompType datatype(sizeof(DatasetElement));
  datatype.insertMember("EXECUTION_BLOCK_ID", HOFFSET(DatasetElement, EXECUTION_BLOCK_ID), H5::PredType::NATIVE_INT);
  datatype.insertMember("SCAN_ID", HOFFSET(DatasetElement, SCAN_ID), H5::PredType::NATIVE_INT);
  datatype.insertMember("BEAM_ID", HOFFSET(DatasetElement, BEAM_ID), H5::PredType::NATIVE_INT);
  datatype.insertMember("T_MIN_MJD", HOFFSET(DatasetElement, T_MIN_MJD), H5::PredType::NATIVE_DOUBLE);
  datatype.insertMember("T_MAX_MJD", HOFFSET(DatasetElement, T_MAX_MJD), H5::PredType::NATIVE_DOUBLE);
  datatype.insertMember("TIME_OFFSET_SECONDS", HOFFSET(DatasetElement, TIME_OFFSET_SECONDS), H5::PredType::NATIVE_DOUBLE);
  datatype.insertMember("NDAT", HOFFSET(DatasetElement, NDAT), H5::PredType::NATIVE_INT);
  datatype.insertMember("FREQ_MHZ", HOFFSET(DatasetElement, FREQ_MHZ), H5::PredType::NATIVE_DOUBLE);
  datatype.insertMember("START_CHAN", HOFFSET(DatasetElement, START_CHAN), H5::PredType::NATIVE_INT);
  datatype.insertMember("BW_MHZ", HOFFSET(DatasetElement, BW_MHZ), H5::PredType::NATIVE_DOUBLE);
  datatype.insertMember("NPOL", HOFFSET(DatasetElement, NPOL), H5::PredType::NATIVE_INT);
  datatype.insertMember("NDIM", HOFFSET(DatasetElement, NDIM), H5::PredType::NATIVE_INT);
  datatype.insertMember("NCHAN_input", HOFFSET(DatasetElement, NCHAN_input), H5::PredType::NATIVE_INT);
  datatype.insertMember("NCHAN_DS", HOFFSET(DatasetElement, NCHAN_DS), H5::PredType::NATIVE_INT);
  datatype.insertMember("NDAT_DS", HOFFSET(DatasetElement, NDAT_DS), H5::PredType::NATIVE_INT);
  datatype.insertMember("NBIN_HIST", HOFFSET(DatasetElement, NBIN_HIST), H5::PredType::NATIVE_INT);
  datatype.insertMember("NBIN_HIST2D", HOFFSET(DatasetElement, NBIN_HIST2D), H5::PredType::NATIVE_INT);
  datatype.insertMember("CHAN_FREQ_MHZ", HOFFSET(DatasetElement, CHAN_FREQ_MHZ), H5::PredType::NATIVE_DOUBLE);

  // Create the dataset with the compound datatype
  H5::DataSpace dataspace(rank, &dims[0]);
  H5::DataSet dataset = file.createDataSet("sample_data", datatype, dataspace);

  // Create a sample data element
  DatasetElement dataPoint = { // NOLINTBEGIN
      12345,         // EXECUTION_BLOCK_ID
      98765,         // SCAN_ID
      1,             // BEAM_ID
      59000.123456,  // T_MIN_MJD
      59000.987654,  // T_MAX_MJD
      1.234,         // TIME_OFFSET_SECONDS
      1000,          // NDAT
      1284.567,      // FREQ_MHZ
      512,           // START_CHAN
      32.768,        // BW_MHZ
      4,             // NPOL
      2,             // NDIM
      8192,          // NCHAN_input
      1024,          // NCHAN_DS
      256,           // NDAT_DS
      100,           // NBIN_HIST
      64,            // NBIN_HIST2D
      1285.122,      // CHAN_FREQ_MHZ
  }; // NOLINTEND

  // Write the data to the dataset
  dataset.write(&dataPoint, datatype);

  /**
   * @brief 
   * 
   */
  // Close the dataset, dataspace, and file
  dataset.close();
  dataspace.close();
  file.close();
}

} // namespace ska::pst::stat::test