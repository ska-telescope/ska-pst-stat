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

#include "ska/pst/common/definitions.h"
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/Timer.h"
#include "ska/pst/common/definitions.h"
#include "ska/pst/stat/tests/ScalarStatPublisherTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <H5Cpp.h>

auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

ScalarStatPublisherTest::ScalarStatPublisherTest()
    : ::testing::Test()
{
}

void ScalarStatPublisherTest::SetUp()
{
  // initialise test harness
  initialise("data_config.txt");
}

void ScalarStatPublisherTest::TearDown()
{
  storage.reset(); // Reset to its initial state
  scalar_stat_publisher.reset();
}

void ScalarStatPublisherTest::populate_storage()
{
  // header values
  populate_1d_vec<double>(storage->channel_centre_frequencies);
  populate_1d_vec<double>(storage->timeseries_bins);
  populate_1d_vec<double>(storage->frequency_bins);

  // data
  populate_2d_vec<float>(storage->mean_frequency_avg);
  populate_2d_vec<float>(storage->mean_frequency_avg_masked);
  populate_2d_vec<float>(storage->variance_frequency_avg);
  populate_2d_vec<float>(storage->variance_frequency_avg_masked);
  populate_3d_vec<float>(storage->mean_spectrum);
  populate_3d_vec<float>(storage->variance_spectrum);
  populate_2d_vec<float>(storage->mean_spectral_power);
  populate_2d_vec<float>(storage->max_spectral_power);
  populate_3d_vec<uint32_t>(storage->histogram_1d_freq_avg);
  populate_3d_vec<uint32_t>(storage->histogram_1d_freq_avg_masked);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_2d_freq_avg);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_2d_freq_avg_masked);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_1d_freq_avg);
  populate_3d_vec<uint32_t>(storage->rebinned_histogram_1d_freq_avg_masked);
  populate_3d_vec<uint32_t>(storage->num_clipped_samples_spectrum);
  populate_2d_vec<uint32_t>(storage->num_clipped_samples);
  populate_3d_vec<float>(storage->spectrogram);
  populate_3d_vec<float>(storage->timeseries);
  populate_3d_vec<float>(storage->timeseries_masked);
}

void ScalarStatPublisherTest::initialise(const std::string& config_file)
{
  config.load_from_file(test_data_file(config_file));
  storage = std::make_shared<ska::pst::stat::StatStorage>(config);

  auto tsamp = config.get_double("TSAMP");
  auto nsamp_per_packet = config.get_uint64("UDP_NSAMP");
  auto total_sample_time = tsamp * ska::pst::common::seconds_per_microseconds * static_cast<double>(nsamp_per_packet);

  storage->set_total_sample_time(total_sample_time);

  auto ntime_bins = config.get_uint32("STAT_REQ_TIME_BINS");
  auto nfreq_bins = config.get_uint32("STAT_REQ_FREQ_BINS");
  storage->resize(ntime_bins, nfreq_bins);

  scalar_stat_publisher = std::make_shared<ska::pst::stat::ScalarStatPublisher>(config, storage);

  populate_storage();
}

TEST_F(ScalarStatPublisherTest, test_construct_delete)
{
  std::shared_ptr<ska::pst::stat::ScalarStatPublisher> ssp = std::make_shared<ska::pst::stat::ScalarStatPublisher>(config, storage);
}

TEST_F(ScalarStatPublisherTest, test_controlled_process_and_read_data)
{
  populate_storage();
  scalar_stat_publisher->publish();
  ska::pst::stat::StatStorage::scalar_stats_t copied_scalar_stat = scalar_stat_publisher->get_scalar_stats();
  ASSERT_EQ(copied_scalar_stat.mean_frequency_avg, storage->mean_frequency_avg);
  ASSERT_EQ(copied_scalar_stat.mean_frequency_avg_masked, storage->mean_frequency_avg_masked);
  ASSERT_EQ(copied_scalar_stat.variance_frequency_avg, storage->variance_frequency_avg);
  ASSERT_EQ(copied_scalar_stat.variance_frequency_avg_masked, storage->variance_frequency_avg_masked);
  ASSERT_EQ(copied_scalar_stat.num_clipped_samples, storage->num_clipped_samples);
  // ASSERT_EQ(copied_scalar_stat.num_clipped_samples_masked, storage->num_clipped_samples_masked);
}

TEST_F(ScalarStatPublisherTest, test_threaded_process_and_read_data)
{
    const int numThreads = 4; // Number of threads for testing
    const int numIterations = 10; // Number of iterations for each thread

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < numIterations; ++j) {
                // Simulate concurrent execution of populate_storage() in multiple threads
                populate_storage();

                // Simulate concurrent execution of publish() in multiple threads
                scalar_stat_publisher->publish();

                // Simulate concurrent execution of get_scalar_stats() in multiple threads
                ska::pst::stat::StatStorage::scalar_stats_t copied_scalar_stat = scalar_stat_publisher->get_scalar_stats();

                // Add assertions to verify that the result matches the expected values
                ASSERT_EQ(copied_scalar_stat.mean_frequency_avg, storage->mean_frequency_avg);
                ASSERT_EQ(copied_scalar_stat.mean_frequency_avg_masked, storage->mean_frequency_avg_masked);
                ASSERT_EQ(copied_scalar_stat.variance_frequency_avg, storage->variance_frequency_avg);
                ASSERT_EQ(copied_scalar_stat.variance_frequency_avg_masked, storage->variance_frequency_avg_masked);
                ASSERT_EQ(copied_scalar_stat.num_clipped_samples, storage->num_clipped_samples);
                // ASSERT_EQ(copied_scalar_stat.num_clipped_samples_masked, storage->num_clipped_samples_masked);
            }
        });
    }

    // Wait for all threads to finish
    for (std::thread& thread : threads) {
        thread.join();
    }
}

}
