# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""Provides tests for loading of HDF5 file."""
import pathlib
from typing import cast

import h5py
import numpy as np
from numpy.testing import assert_allclose
from ska_pst_stat import Statistics
from ska_pst_stat.hdf5 import map_hdf5_key
from ska_pst_stat.utility import Hdf5FileGenerator, StatConfig


def test_load_hdf5_file(
    file_path: pathlib.Path, stat_config: StatConfig, hdf5_file_generator: Hdf5FileGenerator
) -> None:
    """Test that a HDF5 can be loaded successfully."""
    assert not file_path.exists(), "file should not exist"
    hdf5_file_generator.generate()
    generated_stats = hdf5_file_generator.stats

    assert file_path.exists(), "file should exist"

    Statistics.load_from_file(file_path)

    h5_file = h5py.File(file_path, "r")
    keys = {*h5_file.keys()}

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

    header = h5_file["HEADER"][()]
    header_keys = {h for h in header.dtype.names}

    def _assert_data_def(key: str, shape: tuple, dtype: type) -> None:
        data = h5_file[key][...]
        assert data.shape == shape, f"expected shape of {key} to be {shape} but was {data.shape}"
        assert data.dtype == dtype, f"expected type of {key} to be {dtype} but was {data.dtype}"
        stat_key = map_hdf5_key(key)
        stat_data = getattr(generated_stats.data, stat_key)
        assert_allclose(
            data,
            stat_data,
            err_msg=f"Expected hdf5_file[{key}] to have same data as generated_stats.data.{stat_key}",
        )

    def _assert_header_key(key: str) -> None:
        if key == "FILE_FORMAT_VERSION":
            value = h5_file["FILE_FORMAT_VERSION"][()]
        else:
            header = h5_file["HEADER"][0]
            value = header[key]

        if isinstance(value, bytes):
            value = cast(bytes, value).decode("utf-8")

        stat_key = map_hdf5_key(key)
        stat_data = getattr(generated_stats.metadata, stat_key)
        if isinstance(stat_data, np.ndarray):
            assert_allclose(
                value,
                stat_data,
                err_msg=(
                    f"Expected header key '{key}' to have same data as generated_stats.metadata.{stat_key}"
                ),
            )
        else:
            assert (
                value == stat_data
            ), f"Expected header key '{key}' to have same data as generated_stats.metadata.{stat_key}"

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

    # assert header keys. As before ensuring these values match coming from C++ code
    _assert_header_key("FILE_FORMAT_VERSION")
    expected_header_keys = {
        "EB_ID",
        "TELESCOPE",
        "SCAN_ID",
        "BEAM_ID",
        "UTC_START",
        "T_MIN",
        "T_MAX",
        "FREQ",
        "BW",
        "START_CHAN",
        "NPOL",
        "NDIM",
        "NCHAN",
        "NCHAN_DS",
        "NDAT_DS",
        "NBIN_HIST",
        "NREBIN",
        "CHAN_FREQ",
        "FREQUENCY_BINS",
        "TIMESERIES_BINS",
        "NUM_SAMPLES",
        "NUM_SAMPLES_RFI_EXCISED",
        "NUM_SAMPLES_SPECTRUM",
        "NUM_INVALID_PACKETS",
    }

    assert header_keys == expected_header_keys
    for header_key in expected_header_keys:
        _assert_header_key(header_key)
