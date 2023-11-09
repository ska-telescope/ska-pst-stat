# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module provides the ability to generate random data and turn into HDF5 file."""
from __future__ import annotations

import pathlib
from dataclasses import dataclass
from typing import Any, Generator, List

import h5py
import numpy as np
from ska_pst_stat import Statistics
from ska_pst_stat.hdf5.consts import (
    FILE_FORMAT_VERSION_1_0_0,
    HDF5_FILE_FORMAT_VERSION,
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
    HDF5_NUM_CLIPPED_SAMPLES,
    HDF5_NUM_CLIPPED_SAMPLES_RFI_EXCISED,
    HDF5_NUM_CLIPPED_SAMPLES_SPECTRUM,
    HDF5_SPECTROGRAM,
    HDF5_TIMESERIES,
    HDF5_TIMESERIES_RFI_EXCISED,
    HDF5_VARIANCE_FREQUENCY_AVG,
    HDF5_VARIANCE_FREQUENCY_AVG_RFI_EXCISED,
    HDF5_VARIANCE_SPECTRUM,
    TimeseriesDimension,
)
from ska_pst_stat.hdf5.model import HDF5_HEADER_TYPE, StatisticsData, StatisticsMetadata, string_dt


@dataclass(kw_only=True)
class StatConfig:
    """
    A data class used as configuration for generating random data.

    :ivar npol: number of polarisations, default 2.
    :vartype npol: int
    :ivar ndim: number of dimensions in the data, default 2.
    :vartype ndim: int
    :ivar nchan: number of channels in the data, default 432.
    :vartype nchan: int
    :ivar nsamp: number of samples of each channel per heap, default 32.
    :vartype nsamp: int
    :ivar nheap: number of heaps of data to produce, default is 1.
    :vartype nheap: int
    :ivar nbit: the number of bits per data, this can only be 8 or 16.
    :vartype nbit: int
    :ivar nfreq_bins: requested number of frequency bins for spectrogram.
        This gets updated to be a factor of the number of channels.
    :vartype nfreq_bins: int
    :ivar ntime_bins: requested number of temporal bins for spectrogram
        and timeseries. This gets updated to be a factor of the total
        number of samples per channel.
    :vartype ntime_bins: int
    :ivar nrebin: number of bins to use for rebinned histograms
    :vartype nrebin: int
    :ivar sigma: number standard deviations to use to clip data. This
        is only used in the generator.
    :vartype sigma: float
    :ivar freq_mask: the frequency ranges to mask. (Currently not used)
    :vartype freq_mask: str
    :ivar frequency_mhz: the centre frequency for the data as a whole
    :vartype frequency_mhz: float
    :ivar bandwidth_mhz: the bandwidth of data
    :vartype bandwidth_mhz: float
    :ivar start_chan: the starting channel number.
    :vartype start_chan: int
    :ivar tsamp: the time, in microseconds, per sample
    :vartype tsamp: float
    :ivar os_factor: the oversampling factor
    :vartype os_factor: float
    """

    npol: int = 2
    ndim: int = 2
    nchan: int = 432
    nsamp: int = 32
    nheap: int = 1
    nbit: int = 16
    nfreq_bins: int = 36
    ntime_bins: int = 4
    nrebin: int = 256
    sigma: float = 6.0
    freq_mask: str = ""
    frequency_mhz: float = 87.5
    bandwidth_mhz: float = 75.0
    start_chan: int = 0
    tsamp: float = 207.36
    os_factor: float = 4 / 3

    def __post_init__(self: StatConfig) -> None:
        """Ensure configuration is valid."""
        assert self.nbit in [8, 16], "expected nbits to be either 8 or 16"

        self.dtype = np.int8 if self.nbit == 8 else np.int16

        def _recalc_nbins(num_items: int, req_bins: int) -> int:
            if num_items % req_bins == 0:
                return req_bins

            nbin_factor = max(num_items // req_bins, 1)
            while nbin_factor > 1:
                if num_items % nbin_factor == 0:
                    return num_items // nbin_factor

                nbin_factor -= 1

            return num_items

        self.nfreq_bins = _recalc_nbins(self.nchan, self.nfreq_bins)
        self.ntime_bins = _recalc_nbins(self.nheap * self.nsamp, self.ntime_bins)

    @property
    def scale(self: StatConfig) -> float:
        """Get scale of the Gaussian distribution."""
        return self.sigma / self.nbit_limit

    @property
    def nbit_limit(self: StatConfig) -> int:
        """Get the limit for current nbit."""
        return 2 ** (self.nbit - 1)

    @property
    def clipped_low(self: StatConfig) -> int:
        """Get the minimum value for the current nbit."""
        return -self.nbit_limit

    @property
    def clipped_high(self: StatConfig) -> int:
        """Get the maximum value for the current nbit."""
        return self.nbit_limit - 1

    @property
    def rfi_excised_channel_indexes(self: StatConfig) -> List[int]:
        """Get the indexes of the RFI excised channels."""
        return []

    @property
    def non_rfi_channel_indexes(self: StatConfig) -> List[int]:
        """Get the index of channels that are not RFI excised."""
        rfi_excised_channels = self.rfi_excised_channel_indexes
        return [c for c in range(self.nchan) if c not in rfi_excised_channels]

    @property
    def nbin(self: StatConfig) -> int:
        """Get the number of bins for histogram."""
        return 1 << self.nbit

    @property
    def rebin_offset(self: StatConfig) -> int:
        """Get the offset to apply when doing rebinning."""
        return self.nrebin // 2

    @property
    def rebin_max(self: StatConfig) -> int:
        """Get the maximum value after rebinning."""
        return self.nrebin - 1

    @property
    def total_samples_per_channel(self: StatConfig) -> int:
        """Get the total number of samples per channel."""
        return self.nheap * self.nsamp

    @property
    def tsamp_secs(self: StatConfig) -> float:
        """Get the TSAMP value in seconds."""
        return self.tsamp * 1e-6

    @property
    def total_sample_time(self: StatConfig) -> float:
        """Get the total sample time in seconds."""
        return self.tsamp_secs * self.total_samples_per_channel


class Hdf5FileGenerator:
    """Class used to generate a random HD5F statistics file."""

    def __init__(
        self: Hdf5FileGenerator,
        file_path: pathlib.Path | str,
        eb_id: str,
        telescope: str,
        scan_id: int,
        beam_id: str,
        config: StatConfig,
        utc_start: str = "2023-10-23-11:00:00",
    ) -> None:
        """
        Initialise the Hdf5FileGenerator.

        :param file_path: path to the file to create
        :type file_path: pathlib.Path | str
        :param eb_id: the execution block ID of the generated data file
        :type eb_id: str
        :param telescope: the telescope used for the generated data file
        :type telescope: str
        :param scan_id: the scan id for the generated data file
        :type scan_id: int
        :param beam_id: the beam id for the generated data file
        :type beam_id: str
        :param config: the configuration to use to generate the data file
        :type config: StatConfig
        :param utc_start: an ISO formated string of the UTC time at the start of the scan.
        :param utc_start: str
        """
        file_path = pathlib.Path(file_path)
        if not file_path.parent.exists():
            file_path.parent.mkdir(parents=True, exist_ok=True)

        self._file_path = file_path
        self._params: dict = {
            "config": config,
            "eb_id": eb_id,
            "telescope": telescope,
            "scan_id": scan_id,
            "beam_id": beam_id,
            "utc_start": utc_start,
        }
        self._stats: Statistics | None = None

    @property
    def stats(self: Hdf5FileGenerator) -> Statistics:
        """
        Get generatored statistics.

        This will throw an :py:class:`AssertionError` if :py:meth:`generate`
        has not been called.
        """
        assert self._stats is not None, "Statistics has not been generated."
        return self._stats

    def generate(self: Hdf5FileGenerator) -> None:
        """Generate a HDF5 file to use in a test."""
        if self._file_path.exists():
            self._file_path.unlink()

        self._stats = _calc_stats(**self._params)

        metadata = self._stats.metadata

        header_data = np.array(
            [
                (
                    metadata.eb_id,
                    metadata.telescope,
                    metadata.scan_id,
                    metadata.beam_id,
                    metadata.utc_start,
                    metadata.t_min,
                    metadata.t_max,
                    metadata.frequency_mhz,
                    metadata.bandwidth_mhz,
                    metadata.start_chan,
                    metadata.npol,
                    metadata.ndim,
                    metadata.nchan,
                    metadata.nchan_ds,
                    metadata.ndat_ds,
                    metadata.histogram_nbin,
                    metadata.nrebin,
                    metadata.channel_freq_mhz,
                    metadata.frequency_bins,
                    metadata.timeseries_bins,
                    metadata.num_samples,
                    metadata.num_samples_rfi_excised,
                    metadata.num_samples_spectrum,
                    metadata.num_invalid_packets,
                )
            ],
            dtype=HDF5_HEADER_TYPE,
        )

        data = self._stats.data
        with h5py.File(self._file_path, "w") as f:
            file_format_ds = f.create_dataset(HDF5_FILE_FORMAT_VERSION, shape=(), dtype=string_dt)
            file_format_ds[()] = FILE_FORMAT_VERSION_1_0_0

            header_ds = f.create_dataset(HDF5_HEADER, 1, dtype=HDF5_HEADER_TYPE)
            header_ds[...] = header_data

            self._create_data_set(f, HDF5_MEAN_FREQUENCY_AVG, data.mean_frequency_avg)
            self._create_data_set(f, HDF5_MEAN_FREQUENCY_AVG_RFI_EXCISED, data.mean_frequency_avg_rfi_excised)
            self._create_data_set(f, HDF5_VARIANCE_FREQUENCY_AVG, data.variance_frequency_avg)
            self._create_data_set(
                f, HDF5_VARIANCE_FREQUENCY_AVG_RFI_EXCISED, data.variance_frequency_avg_rfi_excised
            )
            self._create_data_set(f, HDF5_MEAN_SPECTRUM, data.mean_spectrum)
            self._create_data_set(f, HDF5_VARIANCE_SPECTRUM, data.variance_spectrum)
            self._create_data_set(f, HDF5_MEAN_SPECTRAL_POWER, data.mean_spectral_power)
            self._create_data_set(f, HDF5_MAX_SPECTRAL_POWER, data.max_spectral_power)
            self._create_data_set(f, HDF5_HISTOGRAM_1D_FREQ_AVG, data.histogram_1d_freq_avg)
            self._create_data_set(
                f, HDF5_HISTOGRAM_1D_FREQ_AVG_RFI_EXCISED, data.histogram_1d_freq_avg_rfi_excised
            )
            self._create_data_set(f, HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG, data.rebinned_histogram_2d_freq_avg)
            self._create_data_set(
                f,
                HDF5_HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_EXCISED,
                data.rebinned_histogram_2d_freq_avg_rfi_excised,
            )
            self._create_data_set(f, HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG, data.rebinned_histogram_1d_freq_avg)
            self._create_data_set(
                f,
                HDF5_HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_EXCISED,
                data.rebinned_histogram_1d_freq_avg_rfi_excised,
            )
            self._create_data_set(f, HDF5_NUM_CLIPPED_SAMPLES_SPECTRUM, data.num_clipped_samples_spectrum)
            self._create_data_set(f, HDF5_NUM_CLIPPED_SAMPLES, data.num_clipped_samples)
            self._create_data_set(
                f, HDF5_NUM_CLIPPED_SAMPLES_RFI_EXCISED, data.num_clipped_samples_rfi_excised
            )
            self._create_data_set(f, HDF5_SPECTROGRAM, data.spectrogram)
            self._create_data_set(f, HDF5_TIMESERIES, data.timeseries)
            self._create_data_set(f, HDF5_TIMESERIES_RFI_EXCISED, data.timeseries_rfi_excised)

    def _create_data_set(
        self: Hdf5FileGenerator,
        file: h5py.File,
        key: str,
        data: np.ndarray,
    ) -> None:
        ds = file.create_dataset(key, data.shape, dtype=data.dtype)
        ds[...] = data


def simple_gaussian_generator(config: StatConfig) -> Generator[np.ndarray, None, None]:
    """Get a generator that can yield Gaussian distributed data based on config."""
    min_value = config.clipped_low
    max_value = config.clipped_high
    while True:
        data = (
            np.random.randn(config.npol, config.ndim, config.nchan, config.total_samples_per_channel)
            / config.scale
        )
        data = np.rint(data)

        data[data < min_value] = min_value
        data[data > max_value] = max_value

        yield data.astype(dtype=config.dtype)


def _calc_stats(
    *args: Any,
    config: StatConfig,
    eb_id: str,
    telescope: str,
    scan_id: int,
    beam_id: str,
    utc_start: str,
    **kwargs: Any,
) -> Statistics:
    """Calculate statistics from random data based on provided config."""
    non_rfi_channel_idx = config.non_rfi_channel_indexes

    num_samples_spectrum = (config.total_samples_per_channel * np.ones(shape=config.nchan)).astype(
        dtype=np.uint32
    )

    num_samples = np.sum(num_samples_spectrum, dtype=np.uint32)
    num_samples_rfi_excised = np.sum(num_samples_spectrum[non_rfi_channel_idx], dtype=np.uint32)
    num_invalid_packets = 0

    # need to calc freq bins
    low_freq = config.frequency_mhz - config.bandwidth_mhz / 2.0
    high_freq = low_freq + config.bandwidth_mhz

    channel_freq_mhz = np.linspace(low_freq, high_freq, num=config.nchan, endpoint=False, dtype=np.float64)
    # this gives us the start freq. Need to have channel centre freq. This offset it BW/nchan/2
    channel_freq_mhz += config.bandwidth_mhz / config.nchan / 2
    freq_bin_factor: int = config.nchan // config.nfreq_bins

    # need to calc freq bins
    frequency_bins = np.linspace(low_freq, high_freq, num=config.nfreq_bins, endpoint=False, dtype=np.float64)
    frequency_bin_bw = config.bandwidth_mhz / config.nfreq_bins
    frequency_bins += frequency_bin_bw / 2

    # need to calc temporal bins
    total_sample_time = config.total_sample_time
    temporal_bin_secs = total_sample_time / config.ntime_bins
    timeseries_bins = (
        np.linspace(0.0, total_sample_time, num=config.ntime_bins, endpoint=False, dtype=np.float64)
        + temporal_bin_secs / 2.0
    )
    temporal_bin_factor = config.total_samples_per_channel // config.ntime_bins

    raw = next(simple_gaussian_generator(config=config))
    assert raw.shape == (config.npol, config.ndim, config.nchan, config.total_samples_per_channel)

    raw_flattened = np.reshape(raw, newshape=(config.npol, config.ndim, -1))
    assert raw_flattened.shape == (config.npol, config.ndim, config.nchan * config.total_samples_per_channel)

    raw_rebinned = np.clip(raw_flattened + config.rebin_offset, 0, config.rebin_max)

    scaled = config.scale * raw
    assert scaled.shape == (config.npol, config.ndim, config.nchan, config.total_samples_per_channel)

    mean_spectrum: np.ndarray = np.mean(scaled, axis=-1, dtype=np.float32)
    assert mean_spectrum.shape == (config.npol, config.ndim, config.nchan)

    mean_frequency_avg: np.ndarray = np.mean(mean_spectrum, axis=-1, dtype=np.float32)
    assert mean_frequency_avg.shape == (config.npol, config.ndim)

    mean_frequency_avg_rfi_excised: np.ndarray = np.mean(
        mean_spectrum[:, :, non_rfi_channel_idx], axis=-1, dtype=np.float32
    )
    assert mean_frequency_avg_rfi_excised.shape == (config.npol, config.ndim)

    variance_spectrum: np.ndarray = np.var(scaled, axis=-1, ddof=1, dtype=np.float32)
    assert variance_spectrum.shape == (config.npol, config.ndim, config.nchan)

    variance_frequency_avg: np.ndarray = np.var(scaled, axis=(-2, -1), ddof=1, dtype=np.float32)
    assert variance_frequency_avg.shape == (config.npol, config.ndim)

    variance_frequency_avg_rfi_excised: np.ndarray = np.var(
        scaled[:, :, non_rfi_channel_idx, :], axis=(-2, -1), ddof=1, dtype=np.float32
    )
    assert variance_frequency_avg_rfi_excised.shape == (config.npol, config.ndim)

    power: np.ndarray = np.sum(scaled**2, axis=1, dtype=np.float32)
    assert power.shape == (config.npol, config.nchan, config.total_samples_per_channel)

    mean_spectral_power: np.ndarray = np.mean(power, axis=-1, dtype=np.float32)
    assert mean_spectral_power.shape == (config.npol, config.nchan)

    max_spectral_power: np.ndarray = np.max(power, axis=-1)
    assert max_spectral_power.shape == (config.npol, config.nchan)

    histogram_1d_freq_avg = np.zeros(shape=(config.npol, config.ndim, config.nbin), dtype=np.uint32)
    histogram_1d_freq_avg_rfi_excised = np.zeros_like(histogram_1d_freq_avg)
    rebinned_histogram_1d_freq_avg = np.zeros(
        shape=(config.npol, config.ndim, config.nrebin), dtype=np.uint32
    )
    rebinned_histogram_1d_freq_avg_rfi_excised = np.zeros_like(rebinned_histogram_1d_freq_avg)

    for ipol in range(config.npol):
        for idim in range(config.ndim):
            histogram_1d_freq_avg[ipol, idim] = np.histogram(
                raw_flattened[ipol, idim],
                bins=config.nbin,
                range=(config.clipped_low, config.clipped_high),
            )[0]
            histogram_1d_freq_avg_rfi_excised[ipol, idim] = np.histogram(
                raw_flattened[ipol, idim, non_rfi_channel_idx],
                bins=config.nbin,
                range=(config.clipped_low, config.clipped_high),
            )[0]
            rebinned_histogram_1d_freq_avg[ipol, idim] = np.histogram(
                raw_rebinned[ipol, idim], bins=config.nrebin, range=(0, config.nrebin - 1)
            )[0]
            rebinned_histogram_1d_freq_avg_rfi_excised[ipol, idim] = np.histogram(
                raw_rebinned[ipol, idim, non_rfi_channel_idx],
                bins=config.nrebin,
                range=(0, config.nrebin - 1),
            )[0]

    rebinned_histogram_2d_freq_avg = np.zeros(
        shape=(config.npol, config.nrebin, config.nrebin), dtype=np.uint32
    )
    rebinned_histogram_2d_freq_avg_rfi_excised = np.zeros_like(rebinned_histogram_2d_freq_avg)

    rebinned_histogram_2d_freq_avg[0] = np.histogram2d(
        raw_rebinned[0, 0],
        raw_rebinned[0, 1],
        bins=config.nrebin,
        range=[[0, config.nrebin - 1], [0, config.nrebin - 1]],
    )[0]
    rebinned_histogram_2d_freq_avg[1] = np.histogram2d(
        raw_rebinned[1, 0],
        raw_rebinned[1, 1],
        bins=config.nrebin,
        range=[[0, config.nrebin - 1], [0, config.nrebin - 1]],
    )[0]
    rebinned_histogram_2d_freq_avg_rfi_excised[0] = np.histogram2d(
        raw_rebinned[0, 0, non_rfi_channel_idx],
        raw_rebinned[0, 1, non_rfi_channel_idx],
        bins=config.nrebin,
        range=[[0, config.nrebin - 1], [0, config.nrebin - 1]],
    )[0]
    rebinned_histogram_2d_freq_avg_rfi_excised[1] = np.histogram2d(
        raw_rebinned[1, 0, non_rfi_channel_idx],
        raw_rebinned[1, 1, non_rfi_channel_idx],
        bins=config.nrebin,
        range=[[0, config.nrebin - 1], [0, config.nrebin - 1]],
    )[0]

    num_clipped_samples_spectrum = np.apply_along_axis(
        lambda x: np.count_nonzero((x <= config.clipped_low) | (x >= config.clipped_high)),
        axis=-1,
        arr=raw,
    ).astype(np.uint32)
    assert num_clipped_samples_spectrum.shape == (config.npol, config.ndim, config.nchan)

    num_clipped_samples = np.sum(num_clipped_samples_spectrum, axis=-1).astype(np.uint32)
    assert num_clipped_samples.shape == (config.npol, config.ndim)

    num_clipped_samples_rfi_excised = np.sum(
        num_clipped_samples_spectrum[:, :, non_rfi_channel_idx], axis=-1
    ).astype(np.uint32)
    assert num_clipped_samples_rfi_excised.shape == (config.npol, config.ndim)

    spectrogram = np.zeros(shape=(config.npol, config.nfreq_bins, config.ntime_bins), dtype=np.float32)
    timeseries = np.zeros(shape=(config.npol, config.ntime_bins, 3), dtype=np.float32)
    timeseries_rfi_excised = np.zeros(shape=(config.npol, config.ntime_bins, 3), dtype=np.float32)

    for (temporal_bin, isamp) in enumerate(range(0, config.total_samples_per_channel, temporal_bin_factor)):
        for (freq_bin, ichan) in enumerate(range(0, config.nchan, freq_bin_factor)):
            # loop over the channel bins and get a slice
            spectrogram_power_slice = power[
                :, ichan : ichan + freq_bin_factor, isamp : isamp + temporal_bin_factor
            ]
            spectrogram[:, freq_bin, temporal_bin] = np.sum(spectrogram_power_slice, axis=(1, 2))

        # all channels over the current temporal bin
        timeseries_power_slice = power[:, :, isamp : isamp + temporal_bin_factor]
        timeseries[:, temporal_bin, TimeseriesDimension.MAX] = np.max(timeseries_power_slice, axis=(1, 2))
        timeseries[:, temporal_bin, TimeseriesDimension.MIN] = np.min(timeseries_power_slice, axis=(1, 2))
        timeseries[:, temporal_bin, TimeseriesDimension.MEAN] = np.mean(timeseries_power_slice, axis=(1, 2))

        # get the power for channels that aren't rfi excised
        timeseries_power_slice_rfi_excised = power[
            :, non_rfi_channel_idx, isamp : isamp + temporal_bin_factor
        ]
        timeseries_rfi_excised[:, temporal_bin, TimeseriesDimension.MAX] = np.max(
            timeseries_power_slice_rfi_excised, axis=(1, 2)
        )
        timeseries_rfi_excised[:, temporal_bin, TimeseriesDimension.MIN] = np.min(
            timeseries_power_slice_rfi_excised, axis=(1, 2)
        )
        timeseries_rfi_excised[:, temporal_bin, TimeseriesDimension.MEAN] = np.mean(
            timeseries_power_slice_rfi_excised, axis=(1, 2)
        )

    metadata = StatisticsMetadata(
        file_format_version="1.0.0",
        eb_id=eb_id,
        telescope=telescope,
        scan_id=scan_id,
        beam_id=beam_id,
        utc_start=utc_start,
        t_min=0,
        t_max=config.total_sample_time,
        frequency_mhz=config.frequency_mhz,
        bandwidth_mhz=config.bandwidth_mhz,
        start_chan=config.start_chan,
        npol=config.npol,
        ndim=config.ndim,
        nchan=config.nchan,
        nchan_ds=config.nfreq_bins,
        ndat_ds=config.ntime_bins,
        histogram_nbin=config.nbin,
        nrebin=config.nrebin,
        channel_freq_mhz=channel_freq_mhz,
        timeseries_bins=timeseries_bins,
        frequency_bins=frequency_bins,
        num_samples=num_samples,
        num_samples_rfi_excised=num_samples_rfi_excised,
        num_samples_spectrum=num_samples_spectrum,
        num_invalid_packets=num_invalid_packets,
    )

    data = StatisticsData(
        mean_frequency_avg=mean_frequency_avg,
        mean_frequency_avg_rfi_excised=mean_frequency_avg_rfi_excised,
        variance_frequency_avg=variance_frequency_avg,
        variance_frequency_avg_rfi_excised=variance_frequency_avg_rfi_excised,
        mean_spectrum=mean_spectrum,
        variance_spectrum=variance_spectrum,
        mean_spectral_power=mean_spectral_power,
        max_spectral_power=max_spectral_power,
        histogram_1d_freq_avg=histogram_1d_freq_avg,
        histogram_1d_freq_avg_rfi_excised=histogram_1d_freq_avg_rfi_excised,
        rebinned_histogram_2d_freq_avg=rebinned_histogram_2d_freq_avg,
        rebinned_histogram_2d_freq_avg_rfi_excised=rebinned_histogram_2d_freq_avg_rfi_excised,
        rebinned_histogram_1d_freq_avg=rebinned_histogram_1d_freq_avg,
        rebinned_histogram_1d_freq_avg_rfi_excised=rebinned_histogram_1d_freq_avg_rfi_excised,
        num_clipped_samples_spectrum=num_clipped_samples_spectrum,
        num_clipped_samples=num_clipped_samples,
        num_clipped_samples_rfi_excised=num_clipped_samples_rfi_excised,
        spectrogram=spectrogram,
        timeseries=timeseries,
        timeseries_rfi_excised=timeseries_rfi_excised,
    )

    return Statistics(metadata=metadata, data=data)
