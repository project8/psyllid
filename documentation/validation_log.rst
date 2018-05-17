Validation Log
==============

Guidelines
----------

* All new features incorporated into a tagged release should have their validation documented.
  * Document the new feature.
  * Perform tests to validate the new feature.
  * If the feature is slated for incorporation into official DAQ processing, perform tests to show that the official configuration works and benefits from this feature.
  * Indicate in this log where to find documentation of the new feature.
  * Indicate in this log what tests were performed, and where to find a writeup of the results.
* Fixes to existing features should also be validated.
  * Perform tests to show that the fix solves the problem that had been indicated.
  * Perform tests to shwo that the fix does not cause other problems.
  * Indicate in this log what tests were performed and how you know the problem was fixed.

Template
--------

Version:
~~~~~~~~

Release Date:
'''''''''''''

New Features:
'''''''''''''

* Feature 1
    * Details
* Feature 2
    * Details

Fixes:
''''''

* Fix 1
    * Details
* Fix 2
    * Details

Log
---

Version: 1.5.0
~~~~~~~~~~~~~~~~~

Release Date: May 8, 2018
'''''''''''''''''''''''''

New Features:
'''''''''''''

* batch_executor retries the reply message's payload and return code; each action happens after the prior one returns (which may not be the conclusion of the action, just like any dripline request)
* frequency mask trigger
    * updated to also output the summed power data in addition to the spline fit used to define the frequency mask. This goes into a second array in the same output file
    * tested using the egg reader and confirming qualitatively that the mask follows the shape of the accumulated power (after normalizing by the number of accumulated points and the mask's offset)
* Dripline-cpp updated to v1.6.0
* CMake option added to allow disabling the FPA on linux builds (useful for batch mode execution without root access).
* server_config now only sets the default authentication file path after checking that the path exists
    * tested via docker batch execution with and without the auth file present; detection and setting appears to work fine
    * TODO! per discussion, we want to change the assumptions about where to look (don't look in home, provide an auth-file path not a filename relative to home)... that assumption is in scarab and so changes will need to be made there before this feature is ready to be added (and before testing can be completed).

Version: 1.4.0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Release Date: April 23, 2018
''''''''''''''''''''''''''''''''''''

New Features:
'''''''''''''

* Egg reader
    * producer node which reads an existing egg file and produces a stream of time_data
    * is a flow controlling node (ie should start paused, is started by dripline commands)
    * intended use case is for reading previously streamed data and testing different trigger configurations
    * has been tested by reading an egg file and producing output files of reasonable size; content of output has not yet been validated
    * validation by using in conjunction with streaming writer and M3Info; printed record content from input file match output file.
    * documentation in doxygen output and node_configuration.rst
* Frequency transform
    * transform node which accepts a time_data stream and produces the same time_data stream and a corresponding freq_data stream
    * intention is that the frequency data match what would be in a ROACH2 frequency packet (as opposed to being the "best possible" FFT of the data, though hopefully those are similar)
    * supports a frequency-only output mode (for building a frequency mask)
    * has been tested only to show that both output streams can be passed to downstream nodes, content validity has not be tested
    * tested by qualitatively looking at a plot of the frequency magnitudes of frequency output file, and also the fft of the original input time data, they looked very similar (up to a normalization factor)
    * documentation in doxygen output and node_configuration.rst
* Streaming frequency writer
    * consumer node which is a direct copy of the streaming_writer node, with time_data replaced with freq_data (ie, it abuses the egg format and puts frequency data into what should be a time record)
    * intended for use only in testing nodes (see above), if a useful feature, the egg format needs to be extended to support it properly and this node modified correspondingly
    * documentation in doxygen output and node_configuration.rst
    * tested as part of the Frequency transoform test above
* tf_roach_receiver optionally always starts on a t packet
    * prior behavior was to start with the next packet received when unpaused; this feature adds a config option which will discard frequency data until the first time data is received (thus ensuring, in principle, that the output is always a matched pair)
    * documentation in doxygen output and node_configuration.rst
* batch_executor control class
    * allows a list of actions to be provided within the master configuration, which specifies a sequence of actions to execute at startup
    * control system modified to allow batch-only mode if the amqp configuration has `make-connection: false`, which will exit after completing batch commands
    * NOTE: currently does not do anything other than print return codes from commands; would be nice to upgrade to check those codes and crash if a command fails
    * tested using a configuration file which configures and uses a frequency mask trigger and event builder
* Dripline-cpp updated to v1.5.0


Version: 1.3.1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Release Date: January 30, 2018
''''''''''''''''''''''''''''''

Fixes:
''''''

* Documentation system update


Version: 1.3.0
~~~~~~~~~~~~~~

Release Date: January 11, 2018
''''''''''''''''''''''''''''''

New Features:
'''''''''''''

* Option to use monarch or not in daq_control
    * Includes dripline get and set functions under the RKS `use-monarch`.
    * API documentation has been updated.
    * If the option is `false` and during a run a writer attempts to write to a Monarch file, Psyllid will crash.
    * Validated by demonstrating that no file is written if the option is `false` (no incoming data; standard streaming 1-channel socket config).
* Auto-building documentation system added
    * Creates a website on readthedocs.org
    * Uses previous documentation content
  
Fixes:
''''''

* Pretrigger implementation in event_builder
    * boost::circular buffer used to implement the pretrigger buffer instead of std::deque.
    * Validated using the ROACH simulator.

* Stream-closing on node exit
    * Writers perform a final attempt to close a stream when they exit.
    * Validated by inserting code to purposefully crash a node.


Version: 1.2.3
~~~~~~~~~~~~~~

Release Date: August 28, 2017
'''''''''''''''''''''''''''''

New Features:
'''''''''''''

* Validation log
    * This file, documentation/validation_log.md, was added to record changes to Psyllid as they're made.
    * No validation is needed as this is not a functional change.
  
Fixes:
''''''

* Propagate missing header values to subsequent files
    * Previously-missing information included voltage offset and range, DAC gain, and frequency min and range.
    * Validated by with a run producing multiple files using the roach_simulator.
  
* Prevent invalid duration setting
    * Setting the duration to 0 caused undefined behavior. This could occur if the value of the duration setting in a dripline request was not an unsigned integer.
    * Now the duration is extracted and checked for validity.  So far it just checks that it's not 0.
    * This was validated by by attempting to set the duration to 0.  It failed, which was a successful test.
