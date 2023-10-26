# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST LMC project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""Provides tests for loading of HDF5 file."""
import pathlib

import h5py
import numpy as np
from ska_pst_stat import Statistics
from ska_pst_stat.utility import Hdf5FileGenerator, StatConfig


def test_load_hdf5_file(
    file_path: pathlib.Path, stat_config: StatConfig, hdf5_file_generator: Hdf5FileGenerator
) -> None:
    """Test that a HDF5 can be loaded successfully."""
    assert not file_path.exists(), "file should not exist"
    hdf5_file_generator.generate()

    assert file_path.exists(), "file should exist"

    Statistics.load_from_file(file_path)

    h5_file = h5py.File(file_path, "r")
    keys = {*h5_file.keys()}
    print(keys)

    # Note not using the constants to ensure these values match coming from C++ code
    expected_keys = {
        "FILE_FORMAT_VERSION",
        "HEADER",
        "MEAN_FREQUENCY_AVG",
        "MEAN_FREQUENCY_AVG_RFI_EXCISED",
        "VARIANCE_FREQUENCY_AVG",
        "VARIANCE_FREQUENCY_AVG_RFI_EXCISED",
        "MEAN_SPECTRUM",
        "VARIANCE_SPECTRUM",
        "MEAN_SPECTRAL_POWER",
        "MAX_SPECTRAL_POWER",
        "HISTOGRAM_1D_FREQ_AVG",
        "HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED",
        "HISTOGRAM_REBINNED_2D_FREQ_AVG",
        "HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED",
        "HISTOGRAM_REBINNED_1D_FREQ_AVG",
        "HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED",
        "NUM_CLIPPED_SAMPLES_SPECTRUM",
        "NUM_CLIPPED_SAMPLES",
        "NUM_CLIPPED_SAMPLES_RFI_EXCISED",
        "SPECTROGRAM",
        "TIMESERIES",
        "TIMESERIES_RFI_EXCISED",
    }
    assert keys == expected_keys

    def _assert_data_def(key: str, shape: tuple, dtype: type) -> None:
        data = h5_file[key][...]
        assert data.shape == shape, f"expected shape of {key} to be {shape} but was {data.shape}"
        assert data.dtype == dtype, f"expected type of {key} to be {dtype} but was {data.dtype}"

    npol = stat_config.npol
    ndim = stat_config.ndim
    nchan = stat_config.nchan
    nrebin = stat_config.nrebin
    nbin = stat_config.nbin
    nfreq_bins = stat_config.nfreq_bins
    ntime_bins = stat_config.ntime_bins

    _assert_data_def("MEAN_FREQUENCY_AVG", (npol, ndim), np.float32)
    _assert_data_def("MEAN_FREQUENCY_AVG_RFI_EXCISED", (npol, ndim), np.float32)
    _assert_data_def("VARIANCE_FREQUENCY_AVG", (npol, ndim), np.float32)
    _assert_data_def("VARIANCE_FREQUENCY_AVG_RFI_EXCISED", (npol, ndim), np.float32)
    _assert_data_def("MEAN_SPECTRUM", (npol, ndim, nchan), np.float32)
    _assert_data_def("VARIANCE_SPECTRUM", (npol, ndim, nchan), np.float32)
    _assert_data_def("MEAN_SPECTRAL_POWER", (npol, nchan), np.float32)
    _assert_data_def("MAX_SPECTRAL_POWER", (npol, nchan), np.float32)
    _assert_data_def("HISTOGRAM_1D_FREQ_AVG", (npol, ndim, nbin), np.uint32)
    _assert_data_def("HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED", (npol, ndim, nbin), np.uint32)
    _assert_data_def("HISTOGRAM_REBINNED_2D_FREQ_AVG", (npol, nrebin, nrebin), np.uint32)
    _assert_data_def("HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED", (npol, nrebin, nrebin), np.uint32)
    _assert_data_def("HISTOGRAM_REBINNED_1D_FREQ_AVG", (npol, ndim, nrebin), np.uint32)
    _assert_data_def("HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED", (npol, ndim, nrebin), np.uint32)
    _assert_data_def("NUM_CLIPPED_SAMPLES_SPECTRUM", (npol, ndim, nchan), np.uint32)
    _assert_data_def("NUM_CLIPPED_SAMPLES", (npol, ndim), np.uint32)
    _assert_data_def("NUM_CLIPPED_SAMPLES_RFI_EXCISED", (npol, ndim), np.uint32)
    _assert_data_def("SPECTROGRAM", (npol, nfreq_bins, ntime_bins), np.float32)
    _assert_data_def("TIMESERIES", (npol, ntime_bins, 3), np.float32)
    _assert_data_def("TIMESERIES_RFI_EXCISED", (npol, ntime_bins, 3), np.float32)
