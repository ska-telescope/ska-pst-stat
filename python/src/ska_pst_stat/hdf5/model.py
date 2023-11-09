# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module defines the model classes when processing HDF5 STAT data."""

from __future__ import annotations

__all__ = [
    "StatisticsData",
    "StatisticsMetadata",
    "HDF5_HEADER_TYPE",
    "map_hdf5_key",
]

from dataclasses import dataclass
from typing import Dict, Literal

import h5py
import nptyping as npt
import numpy as np
from ska_pst_stat.hdf5.consts import (
    HDF5_BEAM_ID,
    HDF5_BW,
    HDF5_CHAN_FREQ,
    HDF5_EB_ID,
    HDF5_FREQ,
    HDF5_FREQUENCY_BINS,
    HDF5_NBIN_HIST,
    HDF5_NCHAN,
    HDF5_NCHAN_DS,
    HDF5_NDAT_DS,
    HDF5_NDIM,
    HDF5_NPOL,
    HDF5_NREBIN,
    HDF5_NUM_INVALID_PACKETS,
    HDF5_NUM_SAMPLES,
    HDF5_NUM_SAMPLES_RFI_EXCISED,
    HDF5_NUM_SAMPLES_SPECTRUM,
    HDF5_SCAN_ID,
    HDF5_START_CHAN,
    HDF5_T_MAX,
    HDF5_T_MIN,
    HDF5_TELESCOPE,
    HDF5_TIMESERIES_BINS,
    HDF5_UTC_START,
)

KEY_MAP: Dict[str, str] = {
    HDF5_BW: "bandwidth_mhz",
    HDF5_FREQ: "frequency_mhz",
    HDF5_NBIN_HIST: "histogram_nbin",
    HDF5_CHAN_FREQ: "channel_freq_mhz",
}


def map_hdf5_key(hdf5_key: str) -> str:
    """Map a key from a HDF5 attribute/dataset to a model dataclass property."""
    try:
        return KEY_MAP[hdf5_key]
    except KeyError:
        return hdf5_key.lower()


string_dt = h5py.string_dtype(encoding="utf-8")
uint32_dt = np.uint32
uint32_array_dt = h5py.vlen_dtype(uint32_dt)
uint64_dt = np.uint64
float_dt = np.float32
double_dt = np.float64
double_array_dt = h5py.vlen_dtype(double_dt)


HDF5_HEADER_TYPE = np.dtype(
    [
        (HDF5_EB_ID, string_dt),
        (HDF5_TELESCOPE, string_dt),
        (HDF5_SCAN_ID, uint64_dt),
        (HDF5_BEAM_ID, string_dt),
        (HDF5_UTC_START, string_dt),
        (HDF5_T_MIN, double_dt),
        (HDF5_T_MAX, double_dt),
        (HDF5_FREQ, double_dt),
        (HDF5_BW, double_dt),
        (HDF5_START_CHAN, uint32_dt),
        (HDF5_NPOL, uint32_dt),
        (HDF5_NDIM, uint32_dt),
        (HDF5_NCHAN, uint32_dt),
        (HDF5_NCHAN_DS, uint32_dt),
        (HDF5_NDAT_DS, uint32_dt),
        (HDF5_NBIN_HIST, uint32_dt),
        (HDF5_NREBIN, uint32_dt),
        (HDF5_CHAN_FREQ, double_array_dt),
        (HDF5_FREQUENCY_BINS, double_array_dt),
        (HDF5_TIMESERIES_BINS, double_array_dt),
        (HDF5_NUM_SAMPLES, uint32_dt),
        (HDF5_NUM_SAMPLES_RFI_EXCISED, uint32_dt),
        (HDF5_NUM_SAMPLES_SPECTRUM, uint32_array_dt),
        (HDF5_NUM_INVALID_PACKETS, uint32_dt),
    ]
)


@dataclass(kw_only=True, frozen=True)
class StatisticsMetadata:
    """
    Data class modeling the metadata from a HDF5 STAT data file.

    :ivar file_format_version: the format of the HDF5 STAT file. Default
        is "1.0.0"
    :vartype file_format_version: str
    :ivar eb_id: the execution block id the file relates to.
    :vartype eb_id: str
    :ivar telescope: the telescope the data were collected for. Should be
        SKALow or SKAMid
    :vartype telescope: str
    :ivar scan_id: the scan id for the generated data file
    :vartype scan_id: int
    :ivar beam_id: the beam id for the generated data file
    :vartype beam_id: str
    :ivar utc_start: the UTC ISO formated start time in of scan to the nearest second.
    :vartype utc_start: str
    :ivar t_min: the time offset, in seconds, from the UTC start time to represent the
        time at the start of the data in the file.
    :vartype t_min: float
    :ivar t_max: the time offset, in seconds, from the UTC start time to represent the
        time at the end of data in the file.
    :vartype t_min: float
    :ivar frequency_mhz: the centre frequency for the data as a whole
    :vartype frequency_mhz: float
    :ivar bandwidth_mhz: the bandwidth of data
    :vartype bandwidth_mhz: float
    :ivar start_chan: the starting channel number.
    :vartype start_chan: int
    :ivar npol: number of polarisations.
    :vartype npol: int
    :ivar ndim: number of dimensions in the data (should be 2 for complex data).
    :vartype ndim: int
    :ivar nchan: number of channels in the data.
    :vartype nchan: int
    :ivar nchan_ds: the number of frequency bins in the spectrogram data.
    :vartype nchan_ds: int
    :ivar ndat_ds: the number of temporal bins in the spectrogram
        and timeseries data.
    :vartype ndat_ds: int
    :ivar histogram_nbin: the number of bins in the histogram data.
    :vartype histogram_nbin: int
    :ivar nrebin: number of bins to use for rebinned histograms
    :vartype nrebin: int
    """

    file_format_version: str = "1.0.0"
    eb_id: str
    telescope: str
    scan_id: int
    beam_id: str
    utc_start: str
    t_min: float
    t_max: float
    frequency_mhz: float
    bandwidth_mhz: float
    start_chan: int
    npol: int
    ndim: int
    nchan: int
    nchan_ds: int
    ndat_ds: int
    histogram_nbin: int
    nrebin: int
    num_samples: int
    num_samples_rfi_excised: int
    num_invalid_packets: int

    @property
    def end_chan(self: StatisticsMetadata) -> int:
        """Get the last channel that the header is for."""
        return self.start_chan + self.nchan - 1


@dataclass(kw_only=True, frozen=True)
class StatisticsData:
    """A data class used to the calculated statistics from random data."""

    channel_freq_mhz: npt.NDArray[Literal["NChan"], npt.Float64]
    mean_frequency_avg: npt.NDArray[Literal["NPol, NDim"], npt.Float32]
    mean_frequency_avg_rfi_excised: npt.NDArray[Literal["NPol, NDim"], npt.Float32]
    variance_frequency_avg: npt.NDArray[Literal["NPol, NDim"], npt.Float32]
    variance_frequency_avg_rfi_excised: npt.NDArray[Literal["NPol, NDim"], npt.Float32]

    mean_spectrum: npt.NDArray[Literal["NPol, NDim, NChan"], npt.Float32]
    variance_spectrum: npt.NDArray[Literal["NPol, NDim, NChan"], npt.Float32]

    mean_spectral_power: npt.NDArray[Literal["NPol, NChan"], npt.Float32]
    max_spectral_power: npt.NDArray[Literal["NPol, NChan"], npt.Float32]

    histogram_1d_freq_avg: npt.NDArray[Literal["NPol, NDim, NBin"], npt.UInt32]
    histogram_1d_freq_avg_rfi_excised: npt.NDArray[Literal["NPol, NDim, NBin"], npt.UInt32]

    rebinned_histogram_2d_freq_avg: npt.NDArray[Literal["NPol, NRebin, NRebin"], npt.UInt32]
    rebinned_histogram_2d_freq_avg_rfi_excised: npt.NDArray[Literal["NPol, NRebin, NRebin"], npt.UInt32]

    rebinned_histogram_1d_freq_avg: npt.NDArray[Literal["NPol, NDim, NRebin"], npt.UInt32]
    rebinned_histogram_1d_freq_avg_rfi_excised: npt.NDArray[Literal["NPol, NDim, NRebin"], npt.UInt32]

    num_clipped_samples_spectrum: npt.NDArray[Literal["NPol, NDim, NChan"], npt.UInt32]
    num_clipped_samples: npt.NDArray[Literal["NPol, NDim"], npt.UInt32]
    num_clipped_samples_rfi_excised: npt.NDArray[Literal["NPol, NDim"], npt.UInt32]

    timeseries_bins: npt.NDArray[Literal["NTimeBin"], npt.Float64]
    frequency_bins: npt.NDArray[Literal["NFreqBin"], npt.Float64]

    spectrogram: npt.NDArray[Literal["NPol, NFreqBin, NTimeBin"], npt.Float32]
    timeseries: npt.NDArray[Literal["NPol, NTimeBin, 3"], npt.Float32]
    timeseries_rfi_excised: npt.NDArray[Literal["NPol, NTimeBin, 3"], npt.Float32]

    num_samples_spectrum: npt.NDArray[Literal["NChan"], npt.UInt32]
