# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST LMC project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module defines the model classes when processing HDF5 STAT data."""

from __future__ import annotations

import pathlib
from dataclasses import dataclass
from typing import Dict, List, Literal

import h5py
import nptyping as npt
import numpy as np
import pandas as pd
from ska_pst_stat.hdf5.consts import (
    HDF5_BEAM_ID,
    HDF5_BW,
    HDF5_CHAN_FREQ,
    HDF5_EB_ID,
    HDF5_FILE_FORMAT_VERSION,
    HDF5_FREQ,
    HDF5_FREQUENCY_BINS,
    HDF5_HEADER,
    HDF5_HISTOGRAM_1D_FREQ_AVG,
    HDF5_HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED,
    HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG,
    HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED,
    HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG,
    HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED,
    HDF5_MAX_SPECTRAL_POWER,
    HDF5_MEAN_FREQUENCY_AVG,
    HDF5_MEAN_FREQUENCY_AVG_RFI_EXCISED,
    HDF5_MEAN_SPECTRAL_POWER,
    HDF5_MEAN_SPECTRUM,
    HDF5_NBIN_HIST,
    HDF5_NCHAN,
    HDF5_NCHAN_DS,
    HDF5_NDAT_DS,
    HDF5_NDIM,
    HDF5_NPOL,
    HDF5_NREBIN,
    HDF5_NUM_CLIPPED_SAMPLES,
    HDF5_NUM_CLIPPED_SAMPLES_RFI_EXCISED,
    HDF5_NUM_CLIPPED_SAMPLES_SPECTRUM,
    HDF5_SCAN_ID,
    HDF5_SPECTROGRAM,
    HDF5_START_CHAN,
    HDF5_T_MAX,
    HDF5_T_MIN,
    HDF5_TELESCOPE,
    HDF5_TIMESERIES,
    HDF5_TIMESERIES_BINS,
    HDF5_TIMESERIES_RFI_EXCISED,
    HDF5_UTC_START,
    HDF5_VARIANCE_FREQUENCY_AVG,
    HDF5_VARIANCE_FREQUENCY_AVG_RFI_EXCISED,
    HDF5_VARIANCE_SPECTRUM,
    Dimension,
    Polarisation,
    TimeseriesDimension,
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
    ]
)

# The following are used as headers within Pandas data frames
POLARISATION = "Polarisation"
DIMENSION = "Dimension"
CHANNEL = "Channel"
TEMPORAL_BIN = "Temporal bin"
BIN = "Bin"
BIN_COUNT = "Count"
RFI_EXCISED = "RFI Excised"
MEAN = "Mean"
MIN = "Min"
MAX = "Max"
VARIANCE = "Var."
CLIPPED = "Clipped"
CHANNEL_FREQ_MHZ = "Channel Freq (MHz)"
TIME_OFFSET = "Time offset"


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


@dataclass(kw_only=True, frozen=True)
class Statistics:
    """
    Data class used to abstract over HDF5 file.

    Instances of this should be created by passing the location of a STAT
    file to the :py:meth:`load_from_file` method.
    """

    metadata: StatisticsMetadata
    data: StatisticsData

    @staticmethod
    def load_from_file(file_path: pathlib.Path | str) -> Statistics:
        """
        Load a HDF5 STAT file and return an instance of the Statistics class.

        :param file_path: the path to the file to load the statistics from
        :type file_path: pathlib.Path | str
        :return: the statistics from the HDF5 file as a Python class
        :rtype: Statistics
        """
        file_path = pathlib.Path(file_path)
        assert file_path.exists(), f"Expected {file_path} to exist."

        with h5py.File(file_path, "r") as f:
            # we only have a size of 1 for header
            file_format_version: h5py.Dataset = f[HDF5_FILE_FORMAT_VERSION][0]
            hdf5_header: h5py.Dataset = f[HDF5_HEADER][0]

            metadata = StatisticsMetadata(
                file_format_version=file_format_version.decode("utf-8"),  # pylint: disable=E1101
                eb_id=hdf5_header[HDF5_EB_ID].decode("utf-8"),  # pylint: disable=E1101
                telescope=hdf5_header[HDF5_TELESCOPE].decode("utf-8"),  # pylint: disable=E1101
                scan_id=hdf5_header[HDF5_SCAN_ID],
                beam_id=hdf5_header[HDF5_BEAM_ID].decode("utf-8"),  # pylint: disable=E1101
                utc_start=hdf5_header[HDF5_UTC_START].decode("utf-8"),  # pylint: disable=E1101
                t_min=hdf5_header[HDF5_T_MIN],
                t_max=hdf5_header[HDF5_T_MAX],
                frequency_mhz=hdf5_header[HDF5_FREQ],
                bandwidth_mhz=hdf5_header[HDF5_BW],
                start_chan=hdf5_header[HDF5_START_CHAN],
                npol=hdf5_header[HDF5_NPOL],
                ndim=hdf5_header[HDF5_NDIM],
                nchan=hdf5_header[HDF5_NCHAN],
                nchan_ds=hdf5_header[HDF5_NCHAN_DS],
                ndat_ds=hdf5_header[HDF5_NDAT_DS],
                histogram_nbin=hdf5_header[HDF5_NBIN_HIST],
                nrebin=hdf5_header[HDF5_NREBIN],
            )

            data = StatisticsData(
                channel_freq_mhz=hdf5_header[HDF5_CHAN_FREQ],
                timeseries_bins=hdf5_header[HDF5_TIMESERIES_BINS],
                frequency_bins=hdf5_header[HDF5_FREQUENCY_BINS],
                mean_frequency_avg=f[HDF5_MEAN_FREQUENCY_AVG][...],
                mean_frequency_avg_rfi_excised=f[HDF5_MEAN_FREQUENCY_AVG_RFI_EXCISED][...],
                variance_frequency_avg=f[HDF5_VARIANCE_FREQUENCY_AVG][...],
                variance_frequency_avg_rfi_excised=f[HDF5_VARIANCE_FREQUENCY_AVG_RFI_EXCISED][...],
                mean_spectrum=f[HDF5_MEAN_SPECTRUM][...],
                variance_spectrum=f[HDF5_VARIANCE_SPECTRUM][...],
                mean_spectral_power=f[HDF5_MEAN_SPECTRAL_POWER][...],
                max_spectral_power=f[HDF5_MAX_SPECTRAL_POWER][...],
                histogram_1d_freq_avg=f[HDF5_HISTOGRAM_1D_FREQ_AVG][...],
                histogram_1d_freq_avg_rfi_excised=f[HDF5_HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED][...],
                rebinned_histogram_2d_freq_avg=f[HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG][...],
                rebinned_histogram_2d_freq_avg_rfi_excised=f[HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED][
                    ...
                ],
                rebinned_histogram_1d_freq_avg=f[HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG][...],
                rebinned_histogram_1d_freq_avg_rfi_excised=f[HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED][
                    ...
                ],
                num_clipped_samples_spectrum=f[HDF5_NUM_CLIPPED_SAMPLES_SPECTRUM][...],
                num_clipped_samples=f[HDF5_NUM_CLIPPED_SAMPLES][...],
                num_clipped_samples_rfi_excised=f[HDF5_NUM_CLIPPED_SAMPLES_RFI_EXCISED][...],
                spectrogram=f[HDF5_SPECTROGRAM][...],
                timeseries=f[HDF5_TIMESERIES][...],
                timeseries_rfi_excised=f[HDF5_TIMESERIES_RFI_EXCISED][...],
            )

            return Statistics(metadata=metadata, data=data)

    @property
    def npol(self: Statistics) -> int:
        """Get the number of polarisations."""
        return self.metadata.npol

    @property
    def ndim(self: Statistics) -> int:
        """
        Get the number of dimensions of voltage data.

        This value should be 2 as SKAO uses complex voltage data and the statistics has real and imaginary
        dimensions.
        """
        return self.metadata.ndim

    @property
    def nchan(self: Statistics) -> int:
        """Get the number of channels for the voltage data."""
        return self.metadata.nchan

    @property
    def header(self: Statistics) -> pd.DataFrame:
        """
        Get the scalar header data for the data file.

        This returns a Pandas data frame of the scalar header data from the HDF5 file. This the user of the
        API to see what is in the HEADER dataset without the need of using a HDF5 view tool

        :return: a human readable version of the header scalar fields.
        :rtype: pd.DataFrame
        """
        data = {
            "Key": [
                "File Format Version",
                "Execution Block ID",
                "Telescope",
                "Scan ID",
                "Beam ID",
                "UTC Start Time",
                "Scan Scan Offset",
                "End Scan Offset",
                "Frequency (MHz)",
                "Bandwidth (MHz)",
                "Start Channel Number",
                "End Channel Number",
                "Num. Polarisations",
                "Num. Dimensions",
                "Num. Channels",
                "Num. Frequency Bins",
                "Num. Temporal Bins",
                "Num. Histogram Bins",
                "Num. Histogram Bins (Rebinned)",
            ],
            "Value": [
                self.metadata.file_format_version,
                self.metadata.eb_id,
                self.metadata.telescope,
                self.metadata.scan_id,
                self.metadata.beam_id,
                self.metadata.utc_start,
                self.metadata.t_min,
                self.metadata.t_max,
                self.metadata.frequency_mhz,
                self.metadata.bandwidth_mhz,
                self.metadata.start_chan,
                self.metadata.end_chan,
                self.metadata.npol,
                self.metadata.ndim,
                self.metadata.nchan,
                self.metadata.nchan_ds,
                self.metadata.ndat_ds,
                self.metadata.histogram_nbin,
                self.metadata.nrebin,
            ],
        }

        return pd.DataFrame(data=data)

    def get_frequency_averaged_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the frequency averaged statistics.

        This will return a data frame that includes statistics across all
        frequencies/channels as well as only the frequencies/channels
        that weren't marked as having RFI.

        While this method is a public method, it is recommened to use
        the following properties directly:

            * :py:attr:`frequency_averaged_stats`
            * :py:attr:`frequency_averaged_stats_rfi_excised`

        The data frame has the following columns:

            * Polarisation - which polarisation that the statistic value is for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * RFI Excised - a boolean value of whether the statistic after RFI
              had been excised.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        The Pandas frame has a MultiIndex key using the ``Polarisation``, ``Dimension``,
        and ``RFI Excised`` columns.
        """
        polarisation: List[str] = []
        dimension: List[str] = []
        rfi_excised: List[bool] = []
        mean_freq_avg: List[float] = []
        variance_freq_avg: List[float] = []
        num_samples_clipped: List[int] = []

        for ipol in range(self.npol):
            for idim in range(self.ndim):
                ipol_text = Polarisation(ipol).text
                idim_text = Dimension(idim).text

                polarisation = [*polarisation, ipol_text, ipol_text]
                dimension = [
                    *dimension,
                    idim_text,
                    idim_text,
                ]
                rfi_excised = [*rfi_excised, False, True]
                mean_freq_avg = [
                    *mean_freq_avg,
                    self.data.mean_frequency_avg[ipol][idim],
                    self.data.mean_frequency_avg_rfi_excised[ipol][idim],
                ]
                variance_freq_avg = [
                    *variance_freq_avg,
                    self.data.variance_frequency_avg[ipol][idim],
                    self.data.variance_frequency_avg_rfi_excised[ipol][idim],
                ]
                num_samples_clipped = [
                    *num_samples_clipped,
                    self.data.num_clipped_samples[ipol][idim],
                    self.data.num_clipped_samples_rfi_excised[ipol][idim],
                ]

        data = {
            POLARISATION: polarisation,
            DIMENSION: dimension,
            RFI_EXCISED: rfi_excised,
            MEAN: mean_freq_avg,
            VARIANCE: variance_freq_avg,
            CLIPPED: num_samples_clipped,
        }

        df = pd.DataFrame(data=data)
        df.set_index([POLARISATION, DIMENSION, RFI_EXCISED], inplace=True)

        return df

    @property
    def frequency_averaged_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the frequency averaged statistics for all frequencies.

        This returns the mean and variance of all the data across
        all frequencies, including frequencies marked as having RFI,
        separated for each polarisation and complex value dimension.
        The statistics also includes the number of samples clipped
        (i.e. the digital value was at the min or max value given the
        number of bits.)

        The data frame has the following columns:

            * Polarisation - which polarisation that the statistic value is for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        The Pandas frame has a MultiIndex key using the ``Polarisation``, and ``Dimension``
        columns.
        """
        df = self.get_frequency_averaged_stats()
        return df.loc[:, :, False]  # type: ignore

    @property
    def frequency_averaged_stats_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the frequency averaged statistics for all frequencies expect those flagged for RFI.

        This returns the mean and variance of all the data across
        all frequencies, expect those flagged for RFI,
        separated for each polarisation and complex value dimension.
        The statistics also includes the number of samples clipped
        (i.e. the digital value was at the min or max value given the
        number of bits.)

        The data frame has the following columns:

            * Polarisation - which polarisation that the statistic value is for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        The Pandas frame has a MultiIndex key using the ``Polarisation``, and ``Dimension``
        columns.
        """
        df = self.get_frequency_averaged_stats()
        return df.loc[:, :, True]  # type: ignore

    def get_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the channel statistics.

        While this method is a public method, it is recommened to use
        the following properties as they provide more specific access to
        the data based on polarisation and specific dimension of the
        complex voltage data.

            * :py:attr:`pol_a_channel_stats`
            * :py:attr:`pol_b_channel_stats`
            * :py:attr:`pol_a_real_channel_stats`
            * :py:attr:`pol_a_imag_channel_stats`
            * :py:attr:`pol_b_real_channel_stats`
            * :py:attr:`pol_b_imag_channel_stats`

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Polarisation - which polarisation that the statistic value is for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        The Pandas frame has a MultiIndex key using the ``Channel``, ``Polarisation``,
        and ``Dimension`` columns.

        :return: a data frame with statistics for each channel split by polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        shape = self.data.mean_spectrum.shape

        channel_number_arange = np.arange(self.metadata.start_chan, self.metadata.end_chan + 1)
        channel_number = np.repeat(channel_number_arange, 4)
        channel_freq_mhz = np.repeat(self.data.channel_freq_mhz, 4)

        polarisation = np.empty(shape=shape, dtype=object)
        polarisation[0, :, :] = Polarisation.POL_A.text
        polarisation[1, :, :] = Polarisation.POL_B.text

        dimension = np.empty(shape=shape, dtype=object)
        dimension[:, 0, :] = Dimension.REAL.text
        dimension[:, 1, :] = Dimension.IMAG.text

        mean_data = self.data.mean_spectrum
        variance_data = self.data.variance_spectrum
        clipped_data = self.data.num_clipped_samples_spectrum

        data = {
            CHANNEL: channel_number,
            POLARISATION: polarisation.flatten(order="F"),
            DIMENSION: dimension.flatten(order="F"),
            CHANNEL_FREQ_MHZ: channel_freq_mhz,
            MEAN: mean_data.flatten(order="F"),
            VARIANCE: variance_data.flatten(order="F"),
            CLIPPED: clipped_data.flatten(order="F"),
        }

        df = pd.DataFrame(data=data)
        df.set_index([CHANNEL, POLARISATION, DIMENSION], inplace=True)
        return df

    @property
    def frequency_bins(self: Statistics) -> npt.NDArray[Literal["NFreqBin"], npt.Float64]:
        """Get the frequency bins used in the spectrogram data."""
        return self.data.frequency_bins

    @property
    def timeseries_bins(self: Statistics) -> npt.NDArray[Literal["NTimeBin"], npt.Float64]:
        """Get the timeseries bins used in the spectrogram and timeseries data."""
        return self.data.timeseries_bins

    @property
    def pol_a_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the polarisation A channel statistics.

        This property incluses both the real and complex dimension
        of the data. The following utility properties are provided
        to get the statistics of each dimension directly:

            * :py:attr:`pol_a_real_channel_stats`
            * :py:attr:`pol_a_imag_channel_stats`

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        The Pandas frame has a MultiIndex key using the ``Channel``, and ``Dimension`` columns.

        :return: a data frame with statistics for each channel split by polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        return df.loc[:, Polarisation.POL_A.text, :]  # type: ignore

    @property
    def pol_a_real_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the real valued, polarisation A channel statistics.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        :return: a data frame with statistics for each channel split by polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        df = df.loc[:, Polarisation.POL_A.text, Dimension.REAL.text]  # type: ignore
        df.reset_index(inplace=True)
        return df

    @property
    def pol_a_imag_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the imaginary valued, polarisation A channel statistics.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        :return: a data frame with statistics for each channel split for polarisation A
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        df = df.loc[:, Polarisation.POL_A.text, Dimension.IMAG.text]  # type: ignore
        df.reset_index(inplace=True)
        return df

    @property
    def pol_b_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the polarisation B channel statistics.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        The Pandas frame has a MultiIndex key using the ``Channel``, and ``Dimension`` columns.

        :return: a data frame with statistics for each channel split by polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        return df.loc[:, Polarisation.POL_B.text, :]  # type: ignore

    @property
    def pol_b_real_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the real valued, polarisation B channel statistics.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        :return: a data frame with statistics for each channel split by polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        df = df.loc[:, Polarisation.POL_B.text, Dimension.REAL.text]  # type: ignore
        df.reset_index(inplace=True)
        return df

    @property
    def pol_b_imag_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the imaginary valued, polarisation B channel statistics.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Channel Freq. (MHz) - the centre frequency for the channel.
            * Mean - the mean of the data for each polarisation and dimension, averaged
              over all channels.
            * Variance - the variance of the data for each polarisation and dimension,
              averaged over all channels.
            * Clipped - number of clipped input samples (maximum level) for each
              polarisation, dimension, averaged over all channels.

        :return: a data frame with statistics for each channel split by polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        df = df.loc[:, Polarisation.POL_B.text, Dimension.IMAG.text]  # type: ignore
        df.reset_index(inplace=True)
        return df

    def get_histogram_data(self: Statistics, rfi_excised: bool) -> pd.DataFrame:
        """
        Get the histogram of the input data integer states for each polarisation and dimension.

        While this method is a public method, it is recommened to use one of the
        following 8 properties as they provide the data in a more usable format:

            * :py:attr:`pol_a_real_histogram`
            * :py:attr:`pol_a_imag_histogram`
            * :py:attr:`pol_b_real_histogram`
            * :py:attr:`pol_b_imag_histogram`
            * :py:attr:`pol_a_real_histogram_rfi_excised`
            * :py:attr:`pol_a_imag_histogram_rfi_excised`
            * :py:attr:`pol_b_real_histogram_rfi_excised`
            * :py:attr:`pol_b_imag_histogram_rfi_excised`

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Polarisation - which polarisation that the statistic value is for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * Count - the number/count for the bin.

        The Pandas frame has a MultiIndex key using the ``Bin``, ``Polarisation``,
        and ``Dimension`` columns.

        :param rfi_excised: a bool value to report on all (False) or RFI excised
            (True) data
        :type rfi_excised: True
        :return: a data frame for histogram data split polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        if rfi_excised:
            histogram_data = self.data.histogram_1d_freq_avg_rfi_excised
        else:
            histogram_data = self.data.histogram_1d_freq_avg

        shape = histogram_data.shape
        polarisation = np.empty(shape=shape, dtype=object)
        polarisation[Polarisation.POL_A] = Polarisation.POL_A.text
        polarisation[Polarisation.POL_B] = Polarisation.POL_B.text

        dimension = np.empty_like(polarisation)
        dimension[:, Dimension.REAL] = Dimension.REAL.text
        dimension[:, Dimension.IMAG] = Dimension.IMAG.text

        # This is already flatten in column order
        bins = np.arange(self.metadata.histogram_nbin).repeat(4)

        data = {
            BIN: bins,
            POLARISATION: polarisation.flatten(order="F"),
            DIMENSION: dimension.flatten(order="F"),
            BIN_COUNT: histogram_data.flatten(order="F"),
        }

        df = pd.DataFrame(data=data)
        df.set_index([BIN, POLARISATION, DIMENSION], inplace=True)
        return df

    @property
    def pol_a_real_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the real valued, polarisation A, input data integer states.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for real valued, polarisation A, voltage data.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.REAL.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_a_imag_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the imaginary valued, polarisation A, input data integer states.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for imaginary valued, polarisation A, voltage data.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.IMAG.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_b_real_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the real valued, polarisation B, input data integer states.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for real valued, polarisation B, voltage data.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.REAL.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_b_imag_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the imaginary valued, polarisation B, input data integer states.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for imaginary valued, polarisation B, voltage data.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.IMAG.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_a_real_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the real valued, pol A, input data integer states expect those flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for real valued, polarisation A, voltage data
             expect those flagged for RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.REAL.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_a_imag_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the imag valued, pol A, input data integer states expect those flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for imaginary valued, polarisation A, voltage data
             expect those flagged for RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.IMAG.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_b_real_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the real valued, pol B, input data integer states expect those flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for real valued, polarisation B, voltage data
             expect those flagged for RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.REAL.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    @property
    def pol_b_imag_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the histogram of the imag valued, pol B, input data integer states expect those flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for imaginary valued, polarisation B, voltage data
             expect those flagged for RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.IMAG.text]  # type: ignore
        df = df[BIN_COUNT].to_frame()
        df.reset_index(inplace=True)
        return df

    def get_rebinned_histogram_data(self: Statistics, rfi_excised: bool) -> pd.DataFrame:
        """
        Get rebinned histogram data.

        While this method is a public method, it is recommened to use one of the
        following 8 properties as they provide the data in a more usable format.

            * :py:attr:`pol_a_real_rebinned_histogram`
            * :py:attr:`pol_a_imag_rebinned_histogram`
            * :py:attr:`pol_b_real_rebinned_histogram`
            * :py:attr:`pol_b_imag_rebinned_histogram`
            * :py:attr:`pol_a_real_rebinned_histogram_rfi_excised`
            * :py:attr:`pol_a_imag_rebinned_histogram_rfi_excised`
            * :py:attr:`pol_b_real_rebinned_histogram_rfi_excised`
            * :py:attr:`pol_b_imag_rebinned_histogram_rfi_excised`

        The number of bins that the data has been rebinned to is
        "Num. Histogram Bins (Rebinned)" value found in the :py:attr:`header`.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Polarisation - which polarisation that the statistic value is for.
            * Dimension - which complex dimension/component (i.e. real or imag)
              that the statistic is for.
            * Count - the number/count for the bin

        The Pandas frame has a MultiIndex key using the ``Bin``, ``Polarisation``,
        and ``Dimension`` columns.

        :param rfi_excised: a bool value to report on all (False) or RFI excised
            (True) data
        :type rfi_excised: True
        :return: a data frame for the rebinned histogram data split polarisation
            and complex volatge dimension.
        :rtype: pd.DataFrame
        """
        if rfi_excised:
            histogram_data = self.data.rebinned_histogram_1d_freq_avg_rfi_excised
        else:
            histogram_data = self.data.rebinned_histogram_1d_freq_avg

        shape = histogram_data.shape
        polarisation = np.empty(shape=shape, dtype=object)
        polarisation[Polarisation.POL_A] = Polarisation.POL_A.text
        polarisation[Polarisation.POL_B] = Polarisation.POL_B.text

        dimension = np.empty_like(polarisation)
        dimension[:, Dimension.REAL] = Dimension.REAL.text
        dimension[:, Dimension.IMAG] = Dimension.IMAG.text

        # This is already flatten in column order
        bins = np.arange(self.metadata.nrebin).repeat(4)

        data = {
            BIN: bins,
            POLARISATION: polarisation.flatten(order="F"),
            DIMENSION: dimension.flatten(order="F"),
            BIN_COUNT: histogram_data.flatten(order="F"),
        }

        df = pd.DataFrame(data=data)
        df.set_index([BIN, POLARISATION, DIMENSION], inplace=True)
        return df

    @property
    def pol_a_real_rebinned_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the real valued, pol A.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for real valued, polarisation A.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.REAL.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_a_imag_rebinned_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the imaginary valued, pol A.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for imaginary valued, polarisation A.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.IMAG.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_b_real_rebinned_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the real valued, pol B.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for real valued, polarisation B.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.REAL.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_b_imag_rebinned_histogram(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the imaginary valued, pol B.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for imaginary valued, polarisation B.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=False)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.IMAG.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_a_real_rebinned_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the real valued, pol A except those flagged with RFI.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for real valued, polarisation A
            except those flagged with RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.REAL.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_a_imag_rebinned_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the imag valued, pol A except those flagged with RFI.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for imaginary valued, polarisation A
            except those flagged with RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_A.text, Dimension.IMAG.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_b_real_rebinned_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the real valued, pol B except those flagged with RFI.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for real valued, polarisation B
            except those flagged with RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.REAL.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    @property
    def pol_b_imag_rebinned_histogram_rfi_excised(self: Statistics) -> pd.DataFrame:
        """
        Get the rebinned histogram of the imag valued, pol B except those flagged with RFI.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for rebinned histogram data for imaginary valued, polarisation B
            except those flagged with RFI.
        :rtype: pd.DataFrame
        """
        df = self.get_rebinned_histogram_data(rfi_excised=True)
        df = df.loc[:, Polarisation.POL_B.text, Dimension.IMAG.text]  # type: ignore
        return df[BIN_COUNT].to_frame()

    def get_rebinned_histogram2d_data(
        self: Statistics, rfi_excised: bool, polarisation: Polarisation
    ) -> npt.NDArray[Literal["NRebin, NRebin"], npt.UInt32]:
        """
        Get the 2D histogram data.

        This returns a Numpy array rather than a Pandas Dataframe.

        While this is a public method the following properties should be used
        as they provide a more user friendly API.

            * :py:attr:`pol_a_rebinned_histogram2d`
            * :py:attr:`pol_b_rebinned_histogram2d`
            * :py:attr:`pol_a_rebinned_histogram2d_rfi_excised`
            * :py:attr:`pol_a_rebinned_histogram2d_rfi_excised`

        :param rfi_excised: use the RFI excised data (True) or all data (False)
        :type rfi_excised: bool
        :param polarisaion: which polarisation of the data to use.
        :type polarisation: Polarisation
        """
        if rfi_excised:
            return self.data.rebinned_histogram_2d_freq_avg_rfi_excised[polarisation]
        else:
            return self.data.rebinned_histogram_2d_freq_avg[polarisation]

    @property
    def pol_a_rebinned_histogram2d(self: Statistics) -> npt.NDArray[Literal["NRebin, NRebin"], npt.UInt32]:
        """
        Get the rebinned 2D histogram data for polarisation A.

        This returns a Numpy array with data for all frequencies.

            * the first array dimension is the real valued data.
            * the second array dimension is the imaginary valued data.

        :return: the rebinned 2D histogram data for polarisation A.
        :rtype: np.ndarray
        """
        return self.get_rebinned_histogram2d_data(rfi_excised=False, polarisation=Polarisation.POL_A)

    @property
    def pol_b_rebinned_histogram2d(self: Statistics) -> npt.NDArray[Literal["NRebin, NRebin"], npt.UInt32]:
        """
        Get the rebinned 2D histogram data for polarisation B.

        This returns a Numpy array with data for all frequencies.

            * the first array dimension is the real valued data.
            * the second array dimension is the imaginary valued data.

        :return: the rebinned 2D histogram data for polarisation B.
        :rtype: np.ndarray
        """
        return self.get_rebinned_histogram2d_data(rfi_excised=False, polarisation=Polarisation.POL_B)

    @property
    def pol_a_rebinned_histogram2d_rfi_excised(
        self: Statistics,
    ) -> npt.NDArray[Literal["NRebin, NRebin"], npt.UInt32]:
        """
        Get the rebinned 2D histogram data for polarisation A except frequencies flagged with RFI.

        This returns a Numpy array with data for frequnecies that aren't RFI excised.

            * the first array dimension is the real valued data.
            * the second array dimension is the imaginary valued data.

        :return: the rebinned 2D histogram data for polarisation A.
        :rtype: np.ndarray
        """
        return self.get_rebinned_histogram2d_data(rfi_excised=True, polarisation=Polarisation.POL_A)

    @property
    def pol_b_rebinned_histogram2d_rfi_excised(
        self: Statistics,
    ) -> npt.NDArray[Literal["NRebin, NRebin"], npt.UInt32]:
        """
        Get the rebinned 2D histogram data for polarisation B except frequencies flagged with RFI.

        This returns a Numpy array with data for frequnecies that aren't RFI excised.

            * the first array dimension is the real valued data.
            * the second array dimension is the imaginary valued data.

        :return: the rebinned 2D histogram data for polarisation B.
        :rtype: np.ndarray
        """
        return self.get_rebinned_histogram2d_data(rfi_excised=True, polarisation=Polarisation.POL_B)

    @property
    def pol_a_spectrogram(
        self: Statistics,
    ) -> npt.NDArray[Literal["NFreqBin, NTimeBin"], npt.Float32]:
        """
        Get the spectrogram data for polarisation A.

        This returns a Numpy array that can be used with Matplotlib
        to plot a Spectrogram. The data in the spectogram in binned
        by channel and within time (see "Num. Frequency Bins",
        "Num. Temporal Bins" in py:attr:`header` for more details.)

        :return: the spectrogram data for polarisation A.
        :rtype: np.ndarray
        """
        return self.data.spectrogram[Polarisation.POL_A]

    @property
    def pol_b_spectrogram(
        self: Statistics,
    ) -> npt.NDArray[Literal["NFreqBin, NTimeBin"], npt.Float32]:
        """
        Get the spectrogram data for polarisation B.

        This returns a Numpy array that can be used with Matplotlib
        to plot a Spectrogram. The data in the spectogram in binned
        by channel and within time (see "Num. Frequency Bins",
        "Num. Temporal Bins" in py:attr:`header` for more details.)

        :return: the spectrogram data for polarisation B.
        :rtype: np.ndarray
        """
        return self.data.spectrogram[Polarisation.POL_B]

    def get_timeseries_data(self: Statistics, rfi_excised: bool) -> pd.DataFrame:
        """
        Get the timeseries data.

        While this is a public method, the following properties should be
        used as they provide a more user friendly access to the data.

            * :py:attr:`pol_a_timeseries`
            * :py:attr:`pol_b_timeseries`
            * :py:attr:`pol_a_timeseries_rfi_excised`
            * :py:attr:`pol_b_timeseries_rfi_excised`

        The timeseries is binned in time (see "Num. Temporal Bins" in :py:attr:`header`)
        and is summed over all frequencies. If `rfi_excised` is True
        then the summing happens over the frequency that are not RFI excised.

        The data frame has the following columns:

            * Polarisation - which polarisation that the statistic value is for.
            * Temporal Bin - the time bin.
            * Time Offset - the offset, in seconds, for the current temporal bin.
            * Max - the maximum power recorded in the temporal bin.
            * Min - the minimum power recorded in the temporal bin.
            * Mean - the mean power recorded in the temporal bin.

        The Pandas frame has a MultiIndex key using the ``Polarisation``,
        and `Temporal Bin` columns.

        :param rfi_excised: whether to use all frequencies (False) or those that
            are not marked as having RFI.
        :type rfi_excised: bool
        :return: a data frame with the timeseries statistics.
        :rtype: pd.DataFrame
        """
        if rfi_excised:
            timeseries_data = self.data.timeseries_rfi_excised
        else:
            timeseries_data = self.data.timeseries

        shape = timeseries_data.shape[:-1]

        temporal_bin = np.arange(self.metadata.ndat_ds).repeat(2)

        polarisation = np.empty(shape=shape, dtype=object)
        polarisation[Polarisation.POL_A] = Polarisation.POL_A.text
        polarisation[Polarisation.POL_B] = Polarisation.POL_B.text

        # this will be in column major format
        timeseries_bins = np.repeat(self.data.timeseries_bins, 2)

        max_data = timeseries_data[:, :, TimeseriesDimension.MAX]
        min_data = timeseries_data[:, :, TimeseriesDimension.MIN]
        mean_data = timeseries_data[:, :, TimeseriesDimension.MEAN]

        data = {
            TEMPORAL_BIN: temporal_bin,
            POLARISATION: polarisation.flatten(order="F"),
            TIME_OFFSET: timeseries_bins,
            MAX: max_data.flatten(order="F"),
            MIN: min_data.flatten(order="F"),
            MEAN: mean_data.flatten(order="F"),
        }

        df = pd.DataFrame(data=data)
        df.set_index([POLARISATION, TEMPORAL_BIN], inplace=True)
        return df

    @property
    def pol_a_timeseries(self: Statistics) -> pd.DataFrame:
        """
        Get the timeseries data for polarisation A for all frequencies.

        The timeseries is binned in time (see ``Num. Temporal Bins`` in :py:attr:`header`)
        and is summed over all frequencies.

        The data frame has the following columns:

            * Temporal Bin - the time bin.
            * Time Offset - the offset, in seconds, for the current temporal bin.
            * Max - the maximum power recorded in the temporal bin.
            * Min - the minimum power recorded in the temporal bin.
            * Mean - the mean power recorded in the temporal bin.

        :return: a data frame with the timeseries statistics for polarisation A.
        :rtype: pd.DataFrame
        """
        df = self.get_timeseries_data(rfi_excised=False).loc[Polarisation.POL_A.text]
        return df.to_frame()

    @property
    def pol_b_timeseries(self: Statistics) -> pd.DataFrame:
        """
        Get the timeseries data for polarisation B for all frequencies.

        The timeseries is binned in time (see ``Num. Temporal Bins`` in :py:attr:`header`)
        and is summed over all frequencies.

        The data frame has the following columns:

            * Temporal Bin - the time bin.
            * Time Offset - the offset, in seconds, for the current temporal bin.
            * Max - the maximum power recorded in the temporal bin.
            * Min - the minimum power recorded in the temporal bin.
            * Mean - the mean power recorded in the temporal bin.

        :return: a data frame with the timeseries statistics for polarisation B.
        :rtype: pd.DataFrame
        """
        df = self.get_timeseries_data(rfi_excised=False).loc[Polarisation.POL_B.text]
        return df.to_frame()

    @property
    def pol_a_timeseries_rfi_excised(
        self: Statistics,
    ) -> pd.DataFrame:
        """
        Get the timeseries data for polarisation A for all frequencies except for RFI excised frequencies.

        The timeseries is binned in time (see ``Num. Temporal Bins`` in :py:attr:`header`)
        and is summed over all frequencies.

        The data frame has the following columns:

            * Temporal Bin - the time bin.
            * Time Offset - the offset, in seconds, for the current temporal bin.
            * Max - the maximum power recorded in the temporal bin.
            * Min - the minimum power recorded in the temporal bin.
            * Mean - the mean power recorded in the temporal bin.

        :return: a data frame with the timeseries statistics for polarisation B except for frequencies that
            have been RFI excised.
        :rtype: pd.DataFrame
        """
        df = self.get_timeseries_data(rfi_excised=True).loc[Polarisation.POL_A.text]
        return df.to_frame()

    @property
    def pol_b_timeseries_rfi_excised(
        self: Statistics,
    ) -> pd.DataFrame:
        """
        Get the timeseries data for polarisation B for all frequencies except for RFI excised frequencies.

        The timeseries is binned in time (see ``Num. Temporal Bins`` in :py:attr:`header`)
        and is summed over all frequencies.

        The data frame has the following columns:

            * Temporal Bin - the time bin.
            * Time Offset - the offset, in seconds, for the current temporal bin.
            * Max - the maximum power recorded in the temporal bin.
            * Min - the minimum power recorded in the temporal bin.
            * Mean - the mean power recorded in the temporal bin.

        :return: a data frame with the timeseries statistics for polarisation B except for frequencies that
            have been RFI excised.
        :rtype: pd.DataFrame
        """
        df = self.get_timeseries_data(rfi_excised=True).loc[Polarisation.POL_B.text]
        return df.to_frame()
