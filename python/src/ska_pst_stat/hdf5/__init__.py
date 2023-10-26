# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST LMC project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module is used for handling a HDF5 STAT file."""

__all__ = [
    "Dimension",
    "Polarisation",
    "StatisticsData",
    "StatisticsMetadata",
    "TimeseriesDimension",
    "HDF5_HEADER_TYPE",
    "map_hdf5_key",
]

from .model import StatisticsData, StatisticsMetadata, HDF5_HEADER_TYPE, map_hdf5_key
from .consts import Polarisation, TimeseriesDimension, Dimension
