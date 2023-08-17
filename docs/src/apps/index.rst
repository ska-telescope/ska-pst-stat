
=========================
SKA PST STAT Applications
=========================

ska_pst_stat_info
-----------------

Prints the version of the ska-pst-stat library to stdout.

ska_pst_stat_file_proc
----------------------

Computes data quality statistics for every sample in the specified pair of data and weights files,
as output by the AA0.5 voltage recorder, producing a single HDF5 output file.

    Usage: ska_pst_stat_file_proc -d data -w weights

    -d data     name of data file
    -w weights  name of weights file

The path of the output file is created according to the following rules:

    #. The output filename has the same stem as the data filename with the extension replaced by `h5`; e.g. if the data filename is `something.data` then the output filename is `something.h5`

    #. If the data file is in a parent folder, then the output file is written to the same parent folder with the last sub-folder replaced by `stat` (if necessary, the `stat` sub-folder is created); e.g.

        #. if the data filename is `parent/something.dada` then the output filename is `stat/something.h5`
        
        #. if the data filename is `/absolute/path/to/something.data` then the output filename is `/absolute/path/stat/something.h5`




