# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST LMC project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module defines constants, such as HDF5 keys."""

from __future__ import annotations

from enum import IntEnum
from typing import List

FILE_FORMAT_VERSION_1_0_0 = "1.0.0"


class Polarisation(IntEnum):
    """An enum used to represent polarisation indexes within the data."""

    POL_A = 0
    POL_B = 1

    @property
    def text(self: Polarisation) -> str:
        """
        Map polarisation enum value to text used in data frames.

        :return: 'A' if value is POL_A else 'B'
        :rtype: str
        """
        return "A" if self == Polarisation.POL_A else "B"


class Dimension(IntEnum):
    """An enum used to represent the complex dimension/component within the data."""

    REAL = 0
    IMAG = 1

    @property
    def text(self: Dimension) -> str:
        """
        Map dimension enum value to text used in data frames.

        :return: 'Real' if value is REAL else 'Imag'
        :rtype: str
        """
        return "Real" if self == Dimension.REAL else "Imag"


class TimeseriesDimension(IntEnum):
    """An enum used to represent which index to use for max/min/mean in timeseries data."""

    MAX = 0
    MIN = 1
    MEAN = 2


# Header Key
HDF5_HEADER: str = "HEADER"
HDF5_FILE_FORMAT_VERSION: str = "FILE_FORMAT_VERSION"
HDF5_EB_ID: str = "EB_ID"
HDF5_TELESCOPE: str = "TELESCOPE"
HDF5_SCAN_ID: str = "SCAN_ID"
HDF5_BEAM_ID: str = "BEAM_ID"
HDF5_UTC_START: str = "UTC_START"
HDF5_T_MIN: str = "T_MIN"
HDF5_T_MAX: str = "T_MAX"
HDF5_FREQ: str = "FREQ"
HDF5_BW: str = "BW"
HDF5_START_CHAN: str = "START_CHAN"
HDF5_NPOL: str = "NPOL"
HDF5_NDIM: str = "NDIM"
HDF5_NCHAN: str = "NCHAN"
HDF5_NCHAN_DS: str = "NCHAN_DS"
HDF5_NDAT_DS: str = "NDAT_DS"
HDF5_NBIN_HIST: str = "NBIN_HIST"
HDF5_NREBIN: str = "NREBIN"
HDF5_CHAN_FREQ: str = "CHAN_FREQ"
HDF5_FREQUENCY_BINS: str = "FREQUENCY_BINS"
HDF5_TIMESERIES_BINS: str = "TIMESERIES_BINS"

# Data keys
HDF5_MEAN_FREQUENCY_AVG: str = "MEAN_FREQUENCY_AVG"
HDF5_MEAN_FREQUENCY_AVG_RFI_EXCISED: str = "MEAN_FREQUENCY_AVG_RFI_EXCISED"
HDF5_VARIANCE_FREQUENCY_AVG: str = "VARIANCE_FREQUENCY_AVG"
HDF5_VARIANCE_FREQUENCY_AVG_RFI_EXCISED: str = "VARIANCE_FREQUENCY_AVG_RFI_EXCISED"
HDF5_MEAN_SPECTRUM: str = "MEAN_SPECTRUM"
HDF5_VARIANCE_SPECTRUM: str = "VARIANCE_SPECTRUM"
HDF5_MEAN_SPECTRAL_POWER: str = "MEAN_SPECTRAL_POWER"
HDF5_MAX_SPECTRAL_POWER: str = "MAX_SPECTRAL_POWER"
HDF5_HISTOGRAM_1D_FREQ_AVG: str = "HISTOGRAM_1D_FREQ_AVG"
HDF5_HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED: str = "HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED"
HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG: str = "HISTOGRAM_REBINNED_2D_FREQ_AVG"
HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED: str = "HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED"
HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG: str = "HISTOGRAM_REBINNED_1D_FREQ_AVG"
HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED: str = "HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED"
HDF5_NUM_CLIPPED_SAMPLES_SPECTRUM: str = "NUM_CLIPPED_SAMPLES_SPECTRUM"
HDF5_NUM_CLIPPED_SAMPLES: str = "NUM_CLIPPED_SAMPLES"
HDF5_NUM_CLIPPED_SAMPLES_RFI_EXCISED: str = "NUM_CLIPPED_SAMPLES_RFI_EXCISED"
HDF5_SPECTROGRAM: str = "SPECTROGRAM"
HDF5_TIMESERIES: str = "TIMESERIES"
HDF5_TIMESERIES_RFI_EXCISED: str = "TIMESERIES_RFI_EXCISED"

HDF5_HEADER_KEYS: List[str] = [
    HDF5_FILE_FORMAT_VERSION,
    HDF5_EB_ID,
    HDF5_TELESCOPE,
    HDF5_SCAN_ID,
    HDF5_BEAM_ID,
    HDF5_UTC_START,
    HDF5_T_MIN,
    HDF5_T_MAX,
    HDF5_FREQ,
    HDF5_BW,
    HDF5_START_CHAN,
    HDF5_NPOL,
    HDF5_NDIM,
    HDF5_NCHAN,
    HDF5_NCHAN_DS,
    HDF5_NDAT_DS,
    HDF5_NBIN_HIST,
    HDF5_NREBIN,
    HDF5_CHAN_FREQ,
    HDF5_FREQUENCY_BINS,
    HDF5_TIMESERIES_BINS,
]

HDF5_DATA_KEYS: List[str] = [
    HDF5_MEAN_FREQUENCY_AVG,
    HDF5_MEAN_FREQUENCY_AVG_RFI_EXCISED,
    HDF5_VARIANCE_FREQUENCY_AVG,
    HDF5_VARIANCE_FREQUENCY_AVG_RFI_EXCISED,
    HDF5_MEAN_SPECTRUM,
    HDF5_VARIANCE_SPECTRUM,
    HDF5_MEAN_SPECTRAL_POWER,
    HDF5_MAX_SPECTRAL_POWER,
    HDF5_HISTOGRAM_1D_FREQ_AVG,
    HDF5_HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED,
    HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG,
    HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED,
    HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG,
    HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED,
    HDF5_NUM_CLIPPED_SAMPLES_SPECTRUM,
    HDF5_NUM_CLIPPED_SAMPLES,
    HDF5_NUM_CLIPPED_SAMPLES_RFI_EXCISED,
    HDF5_SPECTROGRAM,
    HDF5_TIMESERIES,
    HDF5_TIMESERIES_RFI_EXCISED,
]
