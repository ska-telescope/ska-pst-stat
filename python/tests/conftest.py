# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""This module defines elements of the pytest test harness shared by all tests."""

from __future__ import annotations

import logging
import pathlib
import random
import string
import tempfile
from datetime import datetime
from random import randint
from typing import Generator

import pytest
from ska_pst_stat.utility.hdf5_file_generator import Hdf5FileGenerator, StatConfig


@pytest.fixture(scope="session")
def logger() -> logging.Logger:
    """Fixture that returns a default logger for tests."""
    logger = logging.getLogger("TEST_LOGGER")
    logger.setLevel(logging.DEBUG)
    return logger


@pytest.fixture
def eb_id() -> str:
    """Return a valid execution block id for test config."""
    rand_char = random.choice(string.ascii_lowercase)
    rand1 = random.randint(0, 999)
    rand2 = random.randint(0, 99999)
    today_str = datetime.today().strftime("%Y%m%d")

    return f"eb-{rand_char}{rand1:03d}-{today_str}-{rand2:05d}"


@pytest.fixture
def scan_id() -> int:
    """Return a random scan id."""
    return randint(1, 1000)


@pytest.fixture
def telescope() -> str:
    """
    Return a random Telescope name.

    This randomly choses between SKALow and SKAMid
    """
    return random.choice(["SKALow", "SKAMid"])


@pytest.fixture
def beam_id() -> str:
    """Return a random Beam ID."""
    return str(randint(1, 16))


@pytest.fixture
def nheap() -> int:
    """
    Return a random number of heaps.

    Generates between 10 and 20
    """
    return randint(10, 20)


@pytest.fixture
def utc_start() -> str:
    """Return the current time as a UTC formated string."""
    return datetime.today().strftime("%Y%m%d")


@pytest.fixture
def nbit(telescope: str) -> int:
    """
    Return the number of bits in the data based on Telescope.

    If telescope = "SKALow" then this returns 16 else 8
    """
    return 16 if telescope == "SKALow" else 8


@pytest.fixture
def nchan(telescope: str) -> int:
    """Return the number of channels based on Telescope."""
    return 432 if telescope == "SKALow" else 555


@pytest.fixture
def stat_config(nbit: int, nchan: int, nheap: int) -> StatConfig:
    """Return an instance of a StatConfig."""
    return StatConfig(
        nbit=nbit,
        nchan=nchan,
        nheap=nheap,
    )


@pytest.fixture
def file_path(eb_id: str, scan_id: int) -> Generator[pathlib.Path, None, None]:
    """Return the file name for the test HDF5 file."""
    tmp_dir = pathlib.Path(tempfile.gettempdir())
    file_path = tmp_dir / f"test_hdf5_{eb_id}_{scan_id}.h5"
    if file_path.exists():
        file_path.unlink()

    yield file_path

    if file_path.exists():
        file_path.unlink()


@pytest.fixture
def hdf5_file_generator(
    file_path: pathlib.Path,
    telescope: str,
    eb_id: str,
    scan_id: int,
    beam_id: str,
    stat_config: StatConfig,
    utc_start: str,
) -> Hdf5FileGenerator:
    """Fixture to return an instance of a Hdf5FileGenerator."""
    return Hdf5FileGenerator(
        file_path=file_path,
        eb_id=eb_id,
        telescope=telescope,
        scan_id=scan_id,
        beam_id=beam_id,
        config=stat_config,
        utc_start=utc_start,
    )
