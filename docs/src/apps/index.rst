
=========================
SKA PST STAT Applications
=========================

ska_pst_stat_info
-----------------

Prints the version of the ska-pst-stat library to stdout.

ska_pst_stat_file_proc
----------------------

Computes data quality statistics for every sample in the specified pair of data and weights files,
as output by the AA0.5 voltage recorder, producing a single HDF5 output file.  By default, the
output file is written

- in a sub-folder of the current working directory named ``stat/``
- with a filename having the same stem as the input data filename
- with the filename extension replaced by ``h5``

    Usage: ska_pst_stat_file_proc -d data -w weights

    -d data     name of data file
    -w weights  name of weights file

