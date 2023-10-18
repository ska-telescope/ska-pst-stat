SKA PST STAT Architecture
=========================

Classes
-------

The following diagram below shows the classes involved in the core software
architecture of the SKA PST STAT component.

.. uml:: class_diagram.puml
  :caption: Class diagram showing main classes involved

StatProcessor
^^^^^^^^^^^^^^

This is the core class to handle the processing of voltage data. It has
been designed to work on data that is either coming from shared memory
ring buffers during a scan or via memory mapped (mmap) files.

Applications, such as *ska_pst_stat_core* or *ska_pst_stat_file_processor*,
that perform statistical calculations will use this class directly
rather than performing their own orchestration.

During instantiation, this class will create a StatStorage instance with
the correct sizes based on configuration. It creates instances of
StatComputer and StatHdf5FileWriter passing along a shared pointer to the
StatStorage instance.

This is not threadsafe, calls to the *process* method should ensure that
the calls to it are threadsafe.

The StatProcessor asserts that there is data at least the length of one
RESOLUTION bytes (i.e. NPOL * NDIM * NBITS * NCHAN * UDP_NSAMP / 8).
If there is a fractional amount it will only calculate the statistics of
an integer multiple of RESOLUTION bytes.

StatComputer
^^^^^^^^^^^^

This class is the main class for performing the statisical computations.

This class is designed to be re-used between different blocks of data
perform a calculation and updates the StatStorage struct.

See the StatHdf5FileWriter section for the output statistics that are calculated.

StatHdf5FileWriter
^^^^^^^^^^^^^^^^^^

A utility class used for writing out the computed statistics to a file
system. Instances of this class are passed a shared *StatsStorage* and the output
path of where to write statistics to.  Every call to *write* will
serialise the *StatStorage* to a new HDF5 file.

HDF5 was chosen given it is an open standard, rather than creating new
structured file format.

HDF5 Data Structure
*******************

The output HDF5 file includes a HEADER section and each computed data structures is
a separate HDF5 dataset.

The header of the HDF5 file includes the following fields:

  * EB_ID - the execution block that the output statistics file is for.
  * SCAN_ID - the scan that the output statistics file is for.
  * BEAM_ID - the beam used to capture the voltage data used in computing the statistics.
  * UTC_START - the start time of the observation as an ISO 8601 string.
  * T_MIN - the fractional offset of a second from UTC_START.
  * T_MAX - the difference between T_MAX and T_MIN is the length of time, in seconds, of the voltage timeseries for which statistics are computed
  * FREQ - the centre frequency, in MHz, for the voltage data.
  * BW - the bandwidth of data, in MHz, for the voltage data.
  * START_CHAN - the starting channel number for the voltage data.  This allows subbands of data to be processed.
  * NPOL - the number of polarisations of the voltage data. For SKA this is always 2.
  * NDIM - the number of dimensions of the voltage data. For SKA this is 2 as the system is recording complex voltage data.
  * NCHAN - the number of channels that is in the voltage data.
  * NCHAN_DS - the number of frequency bins used in the spectrogram output.
  * NDAT_DS - the number of tempral bins used in the spectrogram and timeseries outputs.
  * NBIN_HIST - the number of bins that are used in the 1-dimensional histogram.
  * NREBIN - the number of channel bins used in the re-binned histograms.
  * CHAN_FREQ - an array of centre frequency for each of the channels.
  * FREQUENCY_BINS - the centre frequency, in MHz, for each frequency bin used in the spectrogram.
  * TIMESERIES_BINS - the observation offset, measured in seconds, for each of the tempral bins used in timeseries and spectrograms

The output data of the HDF5 includes the following datasets:

  * MEAN_FREQUENCY_AVG - the mean of the data for each polarisation and dimension, averaged over all channels.
  * MEAN_FREQUENCY_AVG_RFI_MASKED - the mean of the data for each polarisation and dimension, averaged over all channels, expect those flagged for RFI.
  * VARIANCE_FREQUENCY_AVG - the variance of the data for each polarisation and dimension, averaged over all channels.
  * VARIANCE_FREQUENCY_AVG_RFI_MASKED - the variance of the data for each polarisation and dimension, averaged over all channels, expect those flagged for RFI.
  * MEAN_SPECTRUM - the mean of the data for each polarisation, dimension and channel.
  * VARIANCE_SPECTRUM - the variance of the data for each polarisation, dimension and channel.
  * MEAN_SPECTRAL_POWER - mean power of the data for each polarisation and channel.
  * MAX_SPECTRAL_POWER - maximum power of the data for each polarisation and channel.
  * HISTOGRAM_1D_FREQ_AVG - histogram of the input data integer states for each polarisation and dimension, averaged over all channels.
  * HISTOGRAM_1D_FREQ_AVG_RFI_MASKED - histogram of the input data integer states for each polarisation and dimension, averaged over all channels, expect those flagged for RFI.
  * HISTOGRAM_REBINNED_2D_FREQ_AVG - rebinned 2D histogram of the input data integer states for each polarisation, averaged over all channels.
  * HISTOGRAM_REBINNED_2D_FREQ_AVG_RFI_MASKED - rebinned 2D histogram of the input data integer states for each polarisation, averaged over all channels, expect those flagged for RFI.
  * HISTOGRAM_REBINNED_1D_FREQ_AVG - rebinned histogram of the input data integer states for each polarisation and dimension, averaged over all channels
  * HISTOGRAM_REBINNED_1D_FREQ_AVG_RFI_MASKED - rebinned histogram of the input data integer states for each polarisation and dimension, averaged over all channels, expect those flagged for RFI.
  * NUM_CLIPPED_SAMPLES_SPECTRUM - number of clipped input samples (maximum level) for each polarisation, dimension and channel.
  * NUM_CLIPPED_SAMPLES - number of clipped input samples (maximum level) for each polarisation, dimension, averaged over all channels
  * NUM_CLIPPED_SAMPLES_RFI_MASKED - number of clipped input samples (maximum level) for each polarisation, dimension, averaged over all channels, expect those flagged for RFI.
  * SPECTROGRAM - spectrogram of the data for each polarisation, rebinned in frequency to NCHAN_DS bins and in time to NDAT_DS bins.
  * TIMESERIES - time series of the data for each polarisation, rebinned in time to NDAT_DS bins, averaged over all frequency channels. This includes max, min, and mean of the power in each bin.
  * TIMESERIES_RFI_MASKED - time series of the data for each polarisation, rebinned in time to NDAT_DS bins, averaged over all frequency channels, expect those flagged by RFI. This includes max, min, and mean of the power in each bin.

StatStorage
^^^^^^^^^^^

This class provides an abstraction to all of the storaged required to hold
the statistics products computed by the *StatComputer*. The class will be
constructed with configuration parameters stored in a ska::pst::common::AsciiHeader
with the following required parameters:

  * NPOL    Number of polarisations in the input data stream (will always be 2).
  * NDIM    Number of dimensions of each time stample (will always be 2).
  * NCHAN   Number of channels in the input data stream.
  * NBIT    Number of bits per sample in the input data stream.
  * NREBIN  Number of bins in the re-binned input state histograms.

The class provides public methods to resize the storage and to reset all the values
of the storage to zero. As documented in the StatStorage Class API, the class exposes
all of the storage fields as 1, 2 or 3-dimension std::vector attributes of the appropriate
types.

StatApplicationManager
^^^^^^^^^^^^^^^^^^^^^^^

This class is an implemenation of the ska::pst::common::ApplicationManager class
and is used by the *ska_pst_stat_core* process to manage the lifecycle of
configuring the system and performing a scan.

When the application is in a ScanConfigured state this class will have
created an instance of the StatProcessor class which will be used during
a scan to perform the actual calculation of the statistics and writing
the outputs to a file.


FileProcessor
^^^^^^^^^^^^^

This class is used by the *ska_pst_stat_file_processor* application to
process a specific set of data and weights files. When the application
runs it will read a config file into a ska::pst::common::AsciiHeader that
is passed into the constructor of this class. When the instance of this
class is created it will create an instance of both a DataWeightFileBlockLoader and
a StatProcessor

Sequences
---------

Processing of a block of data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. uml:: stats_processor_seq.puml
  :caption: Sequence diagram for processing statistics with the StatProcessor class, common to both StatApplicationManager and FileProcessor sequences

Processing data during a scan
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. uml:: application_manager_seq.puml
  :caption: Sequence diagram for processing statistics during a scan with the StatApplicationManager class

Processing files after a scan
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. uml:: file_proc_seq.puml
  :caption: Sequence diagram for processing statistics from a file using the FileProcessor class

