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

#include <cmath>
#include <vector>
#include <spdlog/spdlog.h>

#include "ska/pst/stat/tests/StatStorageTest.h"
#include "ska/pst/stat/testutils/GtestMain.h"


auto main(int argc, char* argv[]) -> int
{
  return ska::pst::stat::test::gtest_main(argc, argv);
}

namespace ska::pst::stat::test {

StatStorageTest::StatStorageTest()
    : ::testing::Test()
{
}

void StatStorageTest::SetUp()
{
  config.load_from_file(test_data_file("config.txt"));
}

void StatStorageTest::TearDown()
{
}

TEST_F(StatStorageTest, test_construct_delete) // NOLINT
{
  StatStorage storage(config);
  ASSERT_EQ(storage.get_npol(), config.get_uint32("NPOL")); // NOLINT
  ASSERT_EQ(storage.get_ndim(), config.get_uint32("NDIM")); // NOLINT
  ASSERT_EQ(storage.get_nchan(), config.get_uint32("NCHAN")); // NOLINT
  ASSERT_EQ(storage.get_nbin(), static_cast<uint32_t>(round(pow(2, config.get_double("NBIT"))))); // NOLINT
  ASSERT_EQ(storage.get_nrebin(), config.get_uint32("STAT_NREBIN")); // NOLINT
  ASSERT_FALSE(storage.is_storage_resized()); // NOLINT
  ASSERT_FALSE(storage.is_storage_reset()); // NOLINT
}

TEST_F(StatStorageTest, test_resize) // NOLINT
{
  static constexpr uint32_t ntime_bins = 2;
  static constexpr uint32_t nfreq_bins = 3;

  StatStorage storage(config);
  storage.resize(ntime_bins, nfreq_bins);

  ASSERT_TRUE(storage.is_storage_resized()); // NOLINT
  ASSERT_FALSE(storage.is_storage_reset()); // NOLINT
  ASSERT_EQ(storage.get_ntime_bins(), ntime_bins); // NOLINT
  ASSERT_EQ(storage.get_nfreq_bins(), nfreq_bins); // NOLINT

  uint32_t npol = config.get_uint32("NPOL");
  uint32_t ndim = config.get_uint32("NDIM");
  uint32_t nchan = config.get_uint32("NCHAN");
  uint32_t nbin = static_cast<uint32_t>(round(pow(2, config.get_double("NBIT"))));
  uint32_t nrebin = config.get_uint32("STAT_NREBIN");

  // check the vectors have been resized correctly
  check_storage_1d_dims(storage.channel_centre_frequencies, nchan);

  check_storage_2d_dims(storage.mean_frequency_avg, npol, ndim);
  check_storage_2d_dims(storage.mean_frequency_avg_masked, npol, ndim);
  check_storage_2d_dims(storage.variance_frequency_avg, npol, ndim);
  check_storage_2d_dims(storage.variance_frequency_avg_masked, npol, ndim);

  check_storage_3d_dims(storage.mean_spectrum, npol, ndim, nchan);
  check_storage_3d_dims(storage.variance_spectrum, npol, ndim, nchan);

  check_storage_2d_dims(storage.mean_spectral_power, npol, nchan);
  check_storage_2d_dims(storage.max_spectral_power, npol, nchan);

  check_storage_3d_dims(storage.histogram_1d_freq_avg, npol, ndim, nbin);
  check_storage_3d_dims(storage.histogram_1d_freq_avg_masked, npol, ndim, nbin);

  check_storage_3d_dims(storage.rebinned_histogram_2d_freq_avg, npol, nrebin, nrebin);
  check_storage_3d_dims(storage.rebinned_histogram_2d_freq_avg_masked, npol, nrebin, nrebin);

  check_storage_3d_dims(storage.rebinned_histogram_1d_freq_avg, ndim, nrebin, nrebin);
  check_storage_3d_dims(storage.rebinned_histogram_1d_freq_avg_masked, npol, ndim, nrebin);

  check_storage_3d_dims(storage.num_clipped_samples_spectrum, npol, ndim, nchan);
  check_storage_2d_dims(storage.num_clipped_samples, npol, ndim);

  check_storage_1d_dims(storage.timeseries_bins, ntime_bins);
  check_storage_1d_dims(storage.frequency_bins, nfreq_bins);

  check_storage_3d_dims(storage.spectrogram, npol, nfreq_bins, ntime_bins);
  check_storage_3d_dims(storage.timeseries, npol, ntime_bins, storage.get_ntime_vals());
  check_storage_3d_dims(storage.timeseries_masked, npol, ntime_bins, storage.get_ntime_vals());

  check_storage_1d_dims(storage.rfi_mask_lut, nchan);
}

TEST_F(StatStorageTest, test_reset) // NOLINT
{
  static constexpr uint32_t ntime_bins = 4;
  static constexpr uint32_t nfreq_bins = 5;

  StatStorage storage(config);
  storage.resize(ntime_bins, nfreq_bins - 1);
  storage.reset();

  ASSERT_TRUE(storage.is_storage_resized()); // NOLINT
  ASSERT_TRUE(storage.is_storage_reset()); // NOLINT

  storage.resize(ntime_bins, nfreq_bins);
  ASSERT_FALSE(storage.is_storage_reset()); // NOLINT

  storage.reset();
  ASSERT_TRUE(storage.is_storage_reset()); // NOLINT

  // check the vectors have all been zeroed
  check_storage_1d_vals(storage.channel_centre_frequencies);

  check_storage_2d_vals(storage.mean_frequency_avg);
  check_storage_2d_vals(storage.mean_frequency_avg_masked);
  check_storage_2d_vals(storage.variance_frequency_avg);
  check_storage_2d_vals(storage.variance_frequency_avg_masked);

  check_storage_3d_vals(storage.mean_spectrum);
  check_storage_3d_vals(storage.variance_spectrum);

  check_storage_2d_vals(storage.mean_spectral_power);
  check_storage_2d_vals(storage.max_spectral_power);

  check_storage_3d_vals(storage.histogram_1d_freq_avg);
  check_storage_3d_vals(storage.histogram_1d_freq_avg_masked);

  check_storage_3d_vals(storage.rebinned_histogram_2d_freq_avg);
  check_storage_3d_vals(storage.rebinned_histogram_2d_freq_avg_masked);

  check_storage_3d_vals(storage.rebinned_histogram_1d_freq_avg);
  check_storage_3d_vals(storage.rebinned_histogram_1d_freq_avg_masked);

  check_storage_3d_vals(storage.num_clipped_samples_spectrum);
  check_storage_2d_vals(storage.num_clipped_samples);

  check_storage_1d_vals(storage.timeseries_bins);
  check_storage_1d_vals(storage.frequency_bins);

  check_storage_3d_vals(storage.spectrogram);
  check_storage_3d_vals(storage.timeseries);
  check_storage_3d_vals(storage.timeseries_masked);

  check_storage_1d_vals(storage.rfi_mask_lut);
}

} // namespace ska::pst::stat::test