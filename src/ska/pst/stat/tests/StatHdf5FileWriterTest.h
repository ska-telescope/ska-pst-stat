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

#include <random>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatHdf5FileWriter.h"
#include "ska/pst/stat/StatStorage.h"

#ifndef SKA_PST_STAT_TESTS_StatHdf5FileWriterTest_h
#define SKA_PST_STAT_TESTS_StatHdf5FileWriterTest_h

namespace ska::pst::stat::test {

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

      std::shared_ptr<StatStorage> storage{nullptr};

      ska::pst::common::AsciiHeader config;

      void populate_storage();

      void validate_hdf5_file(const std::shared_ptr<H5::H5File>& file);

      std::shared_ptr<StatHdf5FileWriter> writer{nullptr};

    public:
        StatHdf5FileWriterTest();

        ~StatHdf5FileWriterTest() = default;

        void initialise(const std::string& config_file = "data_config.txt");

    private:

      //! random number engine based on Mersenne Twister algorithm
      std::mt19937 generator;

      template<typename T>
      void populate_1d_vec(std::vector<T>& vec)
      {
        std::uniform_int_distribution<uint32_t> uniform_dist(1, 1000);
        for (auto i = 0; i < vec.size(); i++)
        {
          vec[i] = static_cast<T>(uniform_dist(generator));
        }
      }

      template<typename T>
      void populate_2d_vec(std::vector<std::vector<T>>& vec)
      {
        for (auto i = 0; i < vec.size(); i++)
        {
          populate_1d_vec(vec[i]);
        }
      }

      template<typename T>
      void populate_3d_vec(std::vector<std::vector<std::vector<T>>>& vec)
      {
        for (auto i = 0; i < vec.size(); i++)
        {
          populate_2d_vec(vec[i]);
        }
      }

      template<typename T>
      void assert_1d_vec_header(const std::vector<T>& vec, const hvl_t& header_value)
      {
        ASSERT_EQ(header_value.len, vec.size());
        T *data_ptr = reinterpret_cast<T *>(header_value.p);
        std::vector<T> data_vec = std::vector(data_ptr, data_ptr + header_value.len); // NOLINT
        ASSERT_THAT(data_vec, ::testing::ElementsAreArray(vec));
      }

      template<typename T>
      void assert_1d_vec(const std::vector<T>& vec, std::shared_ptr<H5::H5File> file, std::string data_set_name, const H5::PredType& hd5_type)
      {
        SPDLOG_DEBUG("Asserting {} stored in file correctly with dimensions of [{}]", data_set_name, vec.size());
        H5::DataSet dataset = file->openDataSet(data_set_name);
        H5::DataSpace dataspace = dataset.getSpace();

        ASSERT_EQ(1, dataspace.getSimpleExtentNdims());
        hsize_t dims[1];
        dataspace.getSimpleExtentDims(dims);
        ASSERT_EQ(dims[0], vec.size());

        std::vector<T> data_out(dims[0]);
        dataset.read(data_out.data(), hd5_type);

        validate_1d_vec(vec, data_out.data(), dims);
      }

      template<typename T>
      void assert_2d_vec(const std::vector<std::vector<T>>& vec, std::shared_ptr<H5::H5File> file, std::string data_set_name, const H5::PredType& hd5_type)
      {
        SPDLOG_DEBUG("Asserting {} stored in file correctly with dimensions of [{}, {}]", data_set_name, vec.size(), vec[0].size());
        H5::DataSet dataset = file->openDataSet(data_set_name);
        H5::DataSpace dataspace = dataset.getSpace();

        ASSERT_EQ(2, dataspace.getSimpleExtentNdims());
        hsize_t dims[2];
        dataspace.getSimpleExtentDims(dims);
        ASSERT_EQ(dims[0], vec.size());
        ASSERT_EQ(dims[1], vec[0].size());

        std::vector<T> data_out(dims[0] * dims[1]);
        dataset.read(data_out.data(), hd5_type);

        validate_2d_vec(vec, data_out.data(), dims);
      }

      template<typename T>
      void assert_3d_vec(const std::vector<std::vector<std::vector<T>>>& vec, std::shared_ptr<H5::H5File> file, std::string data_set_name, const H5::PredType& hd5_type)
      {
        SPDLOG_DEBUG("Asserting {} stored in file correctly with dimensions of [{}, {}, {}]", data_set_name, vec.size(), vec[0].size(), vec[0][0].size());
        H5::DataSet dataset = file->openDataSet(data_set_name);
        H5::DataSpace dataspace = dataset.getSpace();

        ASSERT_EQ(3, dataspace.getSimpleExtentNdims());
        hsize_t dims[3];
        dataspace.getSimpleExtentDims(dims);
        ASSERT_EQ(dims[0], vec.size());
        ASSERT_EQ(dims[1], vec[0].size());
        ASSERT_EQ(dims[2], vec[0][0].size());

        std::vector<T> data_out(dims[0] * dims[1] * dims[2]);
        dataset.read(data_out.data(), hd5_type);

        validate_3d_vec(vec, data_out.data(), dims);
      }

      template<typename T>
      void validate_1d_vec(const std::vector<T>& vec, T* hdf5_data, hsize_t *dims)
      {
        ASSERT_EQ(vec.size(), dims[0]);
        SPDLOG_DEBUG("validate_1d_vec asserting vector of dimensions [{}]", dims[0]);
        ASSERT_THAT(std::vector<T>(hdf5_data, hdf5_data + dims[0]), ::testing::ElementsAreArray(vec));
      }

      template<typename T>
      void validate_2d_vec(const std::vector<std::vector<T>>& vec, T* hdf5_data, hsize_t *dims)
      {
        ASSERT_EQ(vec.size(), dims[0]);
        ASSERT_EQ(vec[0].size(), dims[1]);

        SPDLOG_DEBUG("validate_2d_vec asserting vector of dimensions [{}, {}]", dims[0], dims[1]);
        uint64_t offset{0};
        for (auto i = 0; i < dims[0]; i++) {
          ASSERT_THAT(std::vector<T>(hdf5_data + offset, hdf5_data + offset + dims[1]), ::testing::ElementsAreArray(vec[i]));
          offset += dims[1];
        }
      }

      template<typename T>
      void validate_3d_vec(const std::vector<std::vector<std::vector<T>>>& vec, T* hdf5_data, hsize_t *dims)
      {
        ASSERT_EQ(vec.size(), dims[0]);
        ASSERT_EQ(vec[0].size(), dims[1]);
        ASSERT_EQ(vec[0][1].size(), dims[2]);

        SPDLOG_DEBUG("validate_3d_vec asserting vector of dimensions [{}, {}, {}]", dims[0], dims[1], dims[2]);
        uint64_t offset{0};
        for (auto i = 0; i < dims[0]; i++) {
          for (auto j = 0; j < dims[1]; j++) {
            ASSERT_THAT(std::vector<T>(hdf5_data + offset, hdf5_data + offset + dims[2]), ::testing::ElementsAreArray(vec[i][j]));
            offset += dims[2];
          }
        }
      }
};

} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatHdf5FileWriterTest_h
