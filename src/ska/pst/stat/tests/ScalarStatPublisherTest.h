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
#include "ska/pst/stat/StatStorage.h"
#include "ska/pst/stat/ScalarStatPublisher.h"

#ifndef SKA_PST_STAT_TESTS_ScalarStatPublisherTest_h
#define SKA_PST_STAT_TESTS_ScalarStatPublisherTest_h

namespace ska::pst::stat::test {

/**
 * @brief Test the ScalarStatPublisher class
 *
 * @details
 *
 */
class ScalarStatPublisherTest : public ::testing::Test
{
    protected:
      void SetUp() override;

      void TearDown() override;

      std::shared_ptr<ska::pst::stat::StatStorage> storage{nullptr};

      ska::pst::common::AsciiHeader config;

      void populate_storage();

      std::shared_ptr<ScalarStatPublisher> scalar_stat_publisher{nullptr};

    public:
      ScalarStatPublisherTest();
      ~ScalarStatPublisherTest() = default;

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

};

} // namespace ska::pst::stat::test

#endif // SKA_PST_STAT_TESTS_ScalarStatPublisherTest_h
