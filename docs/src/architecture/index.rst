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

StatComputer
^^^^^^^^^^^^

This class is the main class for performing the statisical computations.

This class is designed to be re-used between different blocks of data
perform a calculation and updates the StatStorage struct.

TODO - document the stats computed

StatHdf5FileWriter
^^^^^^^^^^^^^^^^^^

A utility class used for writing out the computed statistics to a file
system. Instances of this class are passed a shared *StatsStorage* and the output
path of where to write statistics to.  Every call to *write* will
serialise the *StatStorage* to a new HDF5 file.

HDF5 was chosen given it is an open standard, rather than creating new
structured file format.

TODO - document the structure of the file

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
class is created it will create an instance of both a FileReader and
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

