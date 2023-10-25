# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST LMC project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module for utility class to generate Gaussian random data."""

__all__ = [
    "Hdf5FileGenerator",
    "calc_stats",
    "StatConfig",
]

from .hdf5_file_generator import Hdf5FileGenerator, calc_stats, StatConfig
