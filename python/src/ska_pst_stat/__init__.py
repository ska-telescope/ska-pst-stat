# -*- coding: utf-8 -*-
#
# This file is part of the SKA PST STAT project
#
# Distributed under the terms of the BSD 3-clause new license.
# See LICENSE for more info.
"""
This module for any Python related code for STAT.

The core class in this module is the :py:class:`Statistics` class
which can be used to load a SKA PST STAT HDF5 file and provide
ease of use access to the underlying property.

This package depends on Pandas, Numpy and H5Py and the dependencies
should get installed automatically when installing this package.

The following code snippet demonstrates how to load a file and
get the header metadata of the file and plot a spectrogram.

.. code-block:: python

    from ska_pst_stat import Statistics
    import numpy as np
    import pandas as pd
    import matplotlib.pyplot as plt

    file_path = "/tmp/some-path-to-file/stat.h5"
    stats = Statistics.load_from_file(file_path)

    # this returns a Pandas DataFrame
    header = stats.header

    # to plot the spectrogram for polarisation A
    plt.imshow(stats.pol_a_spectrogram)
    plt.show()
"""

__all__ = ["Statistics"]

from .stats import Statistics
