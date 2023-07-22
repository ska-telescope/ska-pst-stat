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
  const std::string FILE_NAME = "sample.h5";
  const std::string DATASET_NAME = "dataset";
  const int DATASET_SIZE = 5;

  // Create an HDF5 file
  SPDLOG_TRACE("StatHdf5FileWriterTest.test_hdf5_api Create an HDF5 file");
  H5::H5File file(FILE_NAME, H5F_ACC_TRUNC);
  
  // Define the schema
  SPDLOG_TRACE("StatHdf5FileWriterTest.test_hdf5_api Define the schema");
  hsize_t dims[1] = {DATASET_SIZE};
  H5::DataSpace dataSpace(1, dims);
  
  // Create a dataset
  SPDLOG_TRACE("StatHdf5FileWriterTest.test_hdf5_api Create a dataset");
  H5::DataSet dataset = file.createDataSet(DATASET_NAME, H5::PredType::NATIVE_INT, dataSpace);
  
  // Populate the values
  SPDLOG_TRACE("StatHdf5FileWriterTest.test_hdf5_api Populate the values");
  std::vector<int> data(DATASET_SIZE);
  for (int i = 0; i < DATASET_SIZE; ++i) {
      data[i] = i + 1;
  }
  
  // Write data to the dataset
  SPDLOG_TRACE("StatHdf5FileWriterTest.test_hdf5_api Write data to the dataset");
  dataset.write(data.data(), H5::PredType::NATIVE_INT);
  
  // Read the contents of the file
  SPDLOG_TRACE("StatHdf5FileWriterTest.test_hdf5_api Read the contents of the file");
  std::vector<int> readData(DATASET_SIZE);
  dataset.read(readData.data(), H5::PredType::NATIVE_INT);
  
  // Print the contents
  std::cout << "Contents of the dataset:" << std::endl;
  for (int i = 0; i < DATASET_SIZE; ++i) {
      std::cout << readData[i] << " ";
  }
  std::cout << std::endl;
}

} // namespace ska::pst::stat::test