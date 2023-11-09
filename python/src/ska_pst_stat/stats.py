# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module with the Statistics model class."""

from __future__ import annotations

import pathlib
from dataclasses import dataclass
from typing import List, Literal

import h5py
import nptyping as npt
import numpy as np
import pandas as pd
from ska_pst_stat.hdf5 import Dimension, Polarisation, StatisticsData, StatisticsMetadata, TimeseriesDimension
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
    HDF5_NUM_INVALID_PACKETS,
    HDF5_NUM_SAMPLES,
    HDF5_NUM_SAMPLES_RFI_EXCISED,
    HDF5_NUM_SAMPLES_SPECTRUM,
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
            file_format_version: bytes = f[HDF5_FILE_FORMAT_VERSION][()]  # pylint: disable=E1101
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
                channel_freq_mhz=hdf5_header[HDF5_CHAN_FREQ][...],
                timeseries_bins=hdf5_header[HDF5_TIMESERIES_BINS][...],
                frequency_bins=hdf5_header[HDF5_FREQUENCY_BINS][...],
                num_samples=hdf5_header[HDF5_NUM_SAMPLES],
                num_samples_rfi_excised=hdf5_header[HDF5_NUM_SAMPLES_RFI_EXCISED],
                num_samples_spectrum=hdf5_header[HDF5_NUM_SAMPLES_SPECTRUM][...],
                num_invalid_packets=hdf5_header[HDF5_NUM_INVALID_PACKETS],
            )

            data = StatisticsData(
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
    def channel_numbers(self: Statistics) -> npt.NDArray[Literal["NChan"], npt.Int]:
        """Get an array of channel numbers."""
        return np.arange(self.metadata.start_chan, self.metadata.end_chan + 1)

    @property
    def header(self: Statistics) -> pd.DataFrame:
        """
        Get the header metadata for the data file.

        This returns a Pandas data frame of the header data from the HDF5 file. This the user of the
        API to see what is in the HEADER dataset without the need of using a HDF5 view tool

        The header has the following fields:

        .. list-table::
            :header-rows: 1

            * - Key
              - Example
              - Description
            * - File Format Version
              - 1.0.0
              - the version of the SKA PST STAT file format that the file is from.
            * - Execution Block ID
              - eb-m001-20230921-245
              - the execution block ID of the generated data file
            * - Telescope
              - SKALow
              - the telescope used for the generated data file (i.e. SKALow or SKAMid)
            * - Scan ID
              - 42
              - the ID of the scan that the file was generated from.
            * - Beam ID
              - 1
              - the PST BEAM ID that was used for the scan
            * - UTC Start Time
              - 2023-10-23-11:00:00
              - an ISO formated string of the UTC time at the start of the scan
            * - Start Scan Offset
              - 0.0
              - the time offset, in seconds, from the UTC start time to represent the time at the start of
                the data in the file.
            * - End Scan Offset
              - 0.106168
              - the time offset, in seconds, from the UTC start time to represent the time at the end of
                data in the file.
            * - Frequency (MHz)
              - 87.5
              - the centre frequency for the data as a whole
            * - Bandwidth (MHz)
              - 75.0
              - the bandwidth of data
            * - Start Channel Number
              - 0
              - the starting channel number
            * - End Channel Number
              - 431
              - the last channel that the data is for
            * - Num. Polarisations
              - 2
              - number of polarisations, this should be 2
            * - Num. Dimensions
              - 2
              - number of dimensions in the data (should be 2 for complex data)
            * - Num. Channels
              - 432
              - number of channels in the data
            * - Num. Frequency Bins
              - 36
              - he number of frequency bins in the spectrogram data
            * - Num. Temporal Bins
              - 32
              - the number of temporal bins in the spectrogram and timeseries data
            * - Num. Histogram Bins
              - 65536
              - the number of bins in the histogram data
            * - Num. Histogram Bins (Rebinned)
              - 256
              - number of bins to used in the rebinned histograms
            * - Num. Samples
              - 21012480
              - total number of samples used to calculate statistics
            * - Num. Samples (RFI Excised)
              - 19456000
              - total number of samples used to calculate statistics, excluding RFI excised data
            * - Num. Invalid Packets
              - 0
              - total number invalid/dropped packets in the data used to calculate statistics.

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
                "Start Scan Offset",
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
                "Num. Samples",
                "Num. Samples (RFI Excised)",
                "Num. Invalid Packets",
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
                self.metadata.num_samples,
                self.metadata.num_samples_rfi_excised,
                self.metadata.num_invalid_packets,
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
        Get the frequency averaged statistics from all channels not flagged for RFI.

        This returns the mean and variance of all the data across
        all channels, expect those flagged for RFI,
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
            and complex voltage dimension.
        :rtype: pd.DataFrame
        """
        shape = self.data.mean_spectrum.shape

        channel_number_arange = self.channel_numbers
        channel_number = np.repeat(channel_number_arange, 4)
        channel_freq_mhz = np.repeat(self.metadata.channel_freq_mhz, 4)

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
        return self.metadata.frequency_bins

    @property
    def timeseries_bins(self: Statistics) -> npt.NDArray[Literal["NTimeBin"], npt.Float64]:
        """Get the timeseries bins used in the spectrogram and timeseries data."""
        return self.metadata.timeseries_bins

    @property
    def pol_a_channel_stats(self: Statistics) -> pd.DataFrame:
        """
        Get the polarisation A channel statistics.

        This property includes both the real and complex dimension
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

        :return: a data frame of polarisation A with statistics for each channel split complex
            voltage dimension.
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

        :return: a data frame of the real component of polarisation A with statistics for each channel.
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

        :return: a data frame of the imaginary component of polarisation A with statistics for each channel.
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

        This property includes both the real and complex dimension
        of the data. The following utility properties are provided
        to get the statistics of each dimension directly:

            * :py:attr:`pol_b_real_channel_stats`
            * :py:attr:`pol_b_imag_channel_stats`

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

        :return: a data frame of polarisation B with statistics for each channel split complex
            voltage dimension.
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

        :return: a data frame of the real component of polarisation B with statistics for each channel.
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

        :return: a data frame of the imaginary component of polarisation B with statistics for each channel.
        :rtype: pd.DataFrame
        """
        df = self.get_channel_stats()
        df = df.loc[:, Polarisation.POL_B.text, Dimension.IMAG.text]  # type: ignore
        df.reset_index(inplace=True)
        return df

    def get_spectral_power(self: Statistics) -> pd.DataFrame:
        """
        Get the mean and max spectral power values for each channel.

        This data frame includes both the mean and max of the spectral
        power for each channel for all polarisations.

        The following properties are provided for each polarisation:

            * :py:attr:`pol_a_spectral_power`
            * :py:attr:`pol_b_spectral_power`

        The data frame has the following columns:

            * Polarisation - which polarisation that the statistic value is for.
            * Channel - the channel number the statistics are for.
            * Mean - the mean of the spectral power for the current channel.
            * Max - the maximum of the spectral power for the current channel over
              the time sample of the statistics file.

        The Pandas frame has a MultiIndex key using the ``Polarisation``, and ``Channel`` columns.

        :return: the mean and max spectral power values for each channel.
        :rtype: pd.DataFrame
        """
        shape = self.data.mean_spectral_power.shape

        polarisation = np.empty(shape=shape, dtype=object)
        polarisation[Polarisation.POL_A] = Polarisation.POL_A.text
        polarisation[Polarisation.POL_B] = Polarisation.POL_B.text

        channels = np.repeat(self.channel_numbers, self.npol)
        mean_data = self.data.mean_spectral_power
        max_data = self.data.max_spectral_power
        data = {
            POLARISATION: polarisation.flatten(order="F"),
            CHANNEL: channels,
            MEAN: mean_data.flatten(order="F"),
            MAX: max_data.flatten(order="F"),
        }

        df = pd.DataFrame(data=data)
        df.set_index([POLARISATION], inplace=True)
        return df

    @property
    def pol_a_spectral_power(self: Statistics) -> pd.DataFrame:
        """
        Get the mean and max spectral power values for each channel for polarisation A.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Mean - the mean of the spectral power for the current channel.
            * Max - the maximum of the spectral power for the current channel over
              the time sample of the statistics file.

        :return: the mean and max spectral power values for each channel for polarisation A.
        :rtype: pd.DataFrame
        """
        df = self.get_spectral_power().loc[Polarisation.POL_A.text]
        df.reset_index(inplace=True, drop=True)
        return df  # type: ignore

    @property
    def pol_b_spectral_power(self: Statistics) -> pd.DataFrame:
        """
        Get the mean and max spectral power values for each channel for polarisation B.

        The data frame has the following columns:

            * Channel - the channel number the statistics are for.
            * Mean - the mean of the spectral power for the current channel.
            * Max - the maximum of the spectral power for the current channel over
              the time sample of the statistics file.

        :return: the mean and max spectral power values for each channel for polarisation B.
        :rtype: pd.DataFrame
        """
        df = self.get_spectral_power().loc[Polarisation.POL_B.text]
        df.reset_index(inplace=True, drop=True)
        return df  # type: ignore

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
            and complex voltage dimension.
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
        Get the histogram of the real valued, pol A, input data from all channels not flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for real valued, polarisation A, voltage data
             from all channels not flagged for RFI.
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
        Get the histogram of the imag valued, pol A, input data from all channels not flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for imaginary valued, polarisation A, voltage data
             from all channels not flagged for RFI.
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
        Get the histogram of the real valued, pol B, input data from all channels not flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for real valued, polarisation B, voltage data
             from all channels not flagged for RFI.
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
        Get the histogram of the imag valued, pol B, input data from all channels not flagged for RFI.

        The number of bins in the histogram is 2^(number of bits). For 8 bit
        data this is 256 bins and for 16 bit data this is 65536 bins.

        The data frame has the following columns:

            * Bin - the bin for the histogram count.
            * Count - the number/count for the bin

        :return: a data frame for histogram data for imaginary valued, polarisation B, voltage data
             from all channels not flagged for RFI.
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
        ``Num. Histogram Bins (Rebinned)`` value found in the :py:attr:`header`.

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
            and complex voltage dimension.
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
            * :py:attr:`pol_b_rebinned_histogram2d_rfi_excised`

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
        by channel and within time (see ``Num. Frequency Bins``,
        ``Num. Temporal Bins`` in :py:attr:`header` for more details.)

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
        by channel and within time (see ``Num. Frequency Bins``,
        ``Num. Temporal Bins`` in :py:attr:`header` for more details.)

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

        The timeseries is binned in time (see ``Num. Temporal Bins`` in :py:attr:`header`)
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
        timeseries_bins = np.repeat(self.metadata.timeseries_bins, 2)

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
        return self.get_timeseries_data(rfi_excised=False).loc[Polarisation.POL_A.text]  # type: ignore

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
        return self.get_timeseries_data(rfi_excised=False).loc[Polarisation.POL_B.text]  # type: ignore

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
        return self.get_timeseries_data(rfi_excised=True).loc[Polarisation.POL_A.text]  # type: ignore

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
        return self.get_timeseries_data(rfi_excised=True).loc[Polarisation.POL_B.text]  # type: ignore
