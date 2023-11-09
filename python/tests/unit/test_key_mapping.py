# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""Provides tests for key mapping."""
from dataclasses import fields

from ska_pst_stat.hdf5.consts import HDF5_HEADER_KEYS
from ska_pst_stat.hdf5.model import StatisticsMetadata, map_hdf5_key


def test_map_hdf5_key() -> None:
    """Test that the mapping between a HDF5 key exists in Header data class fields."""
    mapped_keys = {map_hdf5_key(k) for k in HDF5_HEADER_KEYS}
    dataclass_keys = {f.name for f in fields(StatisticsMetadata)}
    assert mapped_keys == dataclass_keys
