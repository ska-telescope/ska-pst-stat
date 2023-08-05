
=========================
SKA PST STAT Applications
=========================

ska_pst_stat_info
----------------

Prints the version of the ska-pst-stat library to stdout.

ska_pst_stat_file_proc
----------------------

Computes data quality statistics for every sample in the specified pair of data and weights files,
as output by the AA0.5 voltage recorder, producing a single HDF5 output file.

    Usage: ska_pst_stat_file_proc -c config -d data -w weights

    -c config   name of file processor configuration file
    -d data     name of data file
    -w weights  name of weights file
    -h          print this help text
    -v          verbose output

