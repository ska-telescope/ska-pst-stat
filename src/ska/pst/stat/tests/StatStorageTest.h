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
#include "ska/pst/stat/StatStorage.h"

#ifndef SKA_PST_STAT_TESTS_StatStorageTest_h
#define SKA_PST_STAT_TESTS_StatStorageTest_h

namespace ska::pst::stat::test {

/**
 * @brief Test the StatStorage class
 *
 * @details
 *
 */
class StatStorageTest : public ::testing::Test
{
  protected:
    void SetUp() override;

    void TearDown() override;

  public:
    StatStorageTest();

    ~StatStorageTest() = default;

    ska::pst::common::AsciiHeader config;

    template <typename T>
    void fill_storage_1d(std::vector<T>& vec, T value)
    {
      std::fill(vec.begin(), vec.end(), value);
    }

    template <typename T>
    void fill_storage_2d(std::vector<std::vector<T>>& vec, T value)
    {
      for (uint32_t i=0; i<vec.size(); i++)
      {
        std::fill(vec[i].begin(), vec[i].end(), value);
      }
    }

    template <typename T>
    void fill_storage_3d(std::vector<std::vector<std::vector<T>>>& vec, T value)
    {
      for (uint32_t i=0; i<vec.size(); i++)
      {
        for (uint32_t j=0; j<vec[i].size(); j++)
        {
          std::fill(vec[i][j].begin(), vec[i][j].end(), value);
        }
      }
    }

    template <typename T>
    bool check_storage_1d_dims(const std::vector<T>& vec, uint32_t dim1)
    {
      return (vec.size() == dim1);
    }

    template <typename T>
    bool check_storage_2d_dims(const std::vector<std::vector<T>>& vec, uint32_t dim1, uint32_t dim2)
    {
      bool ok = (vec.size() == dim1);
      for (uint32_t i=0; i<vec.size(); i++)
      {
        ok &= (vec[i].size() == dim2);
      }
      return ok;
    }

    template <typename T>
    bool check_storage_3d_dims(const std::vector<std::vector<T>>& vec, uint32_t dim1, uint32_t dim2, uint32_t dim3)
    {
      bool ok = (vec.size() == dim1);
      for (uint32_t i=0; i<vec.size(); i++)
      {
        ok &= (vec[i].size() == dim2);
        for (uint32_t j=0; j<vec[i].size(); j++)
        {
          ok &= (vec[i][j].size() == dim3);
        }
      }
      return ok;
    }

    template<typename T>
    bool check_storage_1d_vals(const std::vector<T>& vec, T val)
    {
      return std::all_of(vec.begin(), vec.end(), [&](T i) { return i == val; });
    }

    template<typename T>
    bool check_storage_2d_vals(const std::vector<std::vector<T>>& vec, T val)
    {
      bool ok = true;
      for (uint32_t i=0; i<vec.size(); i++)
      {
        ok &= std::all_of(vec[i].begin(), vec[i].end(), [&](T j) { return j == val; });
      }
      return ok;
    }

    template<typename T>
    bool check_storage_3d_vals(const std::vector<std::vector<std::vector<T>>>& vec, T val)
    {
      bool ok = true;
      for (uint32_t i=0; i<vec.size(); i++)
      {
        for (uint32_t j=0; j<vec[i].size(); j++)
        {
          ok |= std::all_of(vec[i][j].begin(), vec[i][j].end(), [&](T k) { return k == val; });
        }
      }
      return ok;
    }

  private:

};

} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_StatStorageTest_h
