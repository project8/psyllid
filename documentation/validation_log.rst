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

Version: 1.12.0
~~~~~~~~~~~~~~~

Release Date: July 25, 2019
'''''''''''''''''''''''''''

New Features:
''''''''''''''

* Unified Docker build, with automation through Travis CI


Version: 1.11.1
~~~~~~~~~~~~~~~

Release Date: July 21, 2019
'''''''''''''''''''''''''''

Fixes:
''''''

* Updated Dripline to v1.11.0
* If butterfly house is being canceled, but streams do not cancel, then does a global cancel.


Version: 1.11.0
~~~~~~~~~~~~~~~

Release Date: June 28, 2019
'''''''''''''''''''''''''''

New Features:
''''''''''''''

* Added the data-producer node

Fixes:
''''''

* Updated Dripline to v1.10.1
* Updated Midge to v3.7.3
* Define Psyllid-specific dripline return codes in Psyllid


Version: 1.10.3
~~~~~~~~~~~~~~~

Release Date: May 30, 2019
''''''''''''''''''''''''''

Fixes:
''''''

* Handle error conditions while recording data


Version: 1.10.2
~~~~~~~~~~~~~~~

Release Date: May 26, 2019
''''''''''''''''''''''''''

Fixes:
''''''

* Fixed return codes in error conditions


Version: 1.10.1
~~~~~~~~~~~~~~~

Release Date: May 23, 2019
''''''''''''''''''''''''''

Fixes:
''''''

* Fixed missing payloads.
* Dripline-cpp updated to v1.9.2


Version: 1.10.0
~~~~~~~~~~~~~~~

Release Date: May 22, 2019
''''''''''''''''''''''''''

New Features:
'''''''''''''

* All submodules updated
    * Dripline-cpp v1.9.1 (now under driplineorg)
    * Midge v3.7.1
    * Monarch v3.5.8
    * Scarab v2.4.7
* New CL syntax
    * Now using the new Scarab CLI framework
    * Standard CL argument format
* Using the updated Dripline-cpp interface
* Using the new Midge template metaprogramming

Fixes:
''''''

* Fixed infinite loop for the startup corner case where `activate-on-startup` is used but no streams are defined.


Version: 1.9.3
~~~~~~~~~~~~~~

Release Date: February 17, 2019
'''''''''''''''''''''''''''''''

Fixes:
''''''

* Fixed missing ampersand in the FMT binding


Version: 1.9.2
~~~~~~~~~~~~~~

Release Date: January 10, 2019
''''''''''''''''''''''''''''''

Fixes:
''''''

* Fixed setting of the run description via dripline


Version: 1.8.3
~~~~~~~~~~~~~~

Release Date: August 27, 2018
'''''''''''''''''''''''''''''

Fixes:
''''''

* Midge update to v3.6.3
    * Missing include fixed
    * Validation: Build now works on Ubuntu system where it failed before


Version 1.8.0
~~~~~~~~~~~~~~~~~~

Release Date: July 27, 2018
'''''''''''''''''''''''''''

New Features:
'''''''''''''

* ids in skip_buffer are written as true when event_builder switches from skipping to untriggered
    * as before, if the capacity of the skip_buffer is greater than the capacity of the pretrigger_buffer only ids that don't fit into pretrigger_buffer are written out as true
    * if the capacity of the skip_buffer is smaller than the capacity of the pretrigger_buffer all ids in the skip_buffer are written out as true
    * tested by running psyllid with the egg3-reader and checking the logging output. No crash occured and the looging output showed that the correct number of ids were written.
* implementing support for both set_condition and batch actions:
    * server_config now defines condition 10 and 12, both call the cmd 'hard-abort'
    * server_config now defines a top-level node 'batch-commands' with an entry for 'hard-abort' which calls 'stop-run'
    * request_receiver stores the above map (configurable in config file as top-level node 'set-conditions'); responds to set-condition commands by calling the mapped rks as an OP_CMD with empty message body
      * this had a bug which is now fixed, it checked for the new name but populated by the old one
    * batch_executor stores the batch-commands map (each entry in the node is an array of commands following the same syntax as those run when the system starts
    * batch_executor's constructor binds request-receiver commands for each key in the above map to do_batch_cmd_request, which adds the configured array of actions to the batch queue. This is called as `agent cmd <queue>.<key>`.
    * batch_executor's execute() method now has an infinite loop option which always tries to empty a concurrent_queue of actions (there are now utility methods plus the above which can populate that queue.
    * run_server's thread execution logic changed to account for the above changes to batch_executor's execute()
    * the 'batch-actions' top-level node name is changed to 'on-startup' to be more clear
    * tested by running psyllid in insectarium and confirming execution of stop run both on `cmd broadcast.set_condition 0` and `cmd psyllid_queue.hard-abort`.
* updating scarab dependency to version v2.1.1
    * tested by running psyllid in insectarium in batch mode
* adding condition_variable notice from daq_control to indicate to request_receiver and batch_executor when the nodes are ready
    * tested by having batch executor use on-start commands that need to talk to nodes (this previously resulted in crashing)

Fixes:
''''''

* corrected compiler warnings related to use of '%u' vs '%lu' for long unsigned ints in testing
* modified tk_spline (external) spline::set_boundary to be inline (it was triggering gcc warnings because it is unused)


Version 1.7.1:
~~~~~~~~~~~~~~

Release Date: July 11, 2018
'''''''''''''''''''''''''''

New Features
''''''''''''
None

Fixes
'''''
* Modified the Frequency Transform node to re-order FFTW output into ascending frequency order (should match Roach packet content order)
    * Tested by making psyllid record and write a frequency mask from frequency data that it produced by reading and fourier-transforming the time series from an egg file. The content of the array is now ordered correctly. This was verified by comparing the mask to the gain variation calculated by Katydid. 


Version 1.7.0:
~~~~~~~~~~~~~~~~~

Release Date: June 27, 2018
'''''''''''''''''''''''''''

New Features:
'''''''''''''

* stream_manager methods for OP_GET of stream and node lists
    * methods added to stream_manager, with extra get bindings in run_server
    * tested by getting each from a running psyllid instance in insectarium and confirming:
        * get stream-list: returns streams
        * get node-list: returns error (need to specify a stream)
        * get node-list.ch0: returns nodes


Version: 1.6.0
~~~~~~~~~~~~~~~~~

Release Date: May 25, 2018
''''''''''''''''''''''''''

New Features:
'''''''''''''

* midge updated to v3.5.4 (updates scarab to v1.6.1)
* server_config now only sets the default authentication file path after checking that the path exists
    * tested via docker batch execution with and without the auth file present; detection and setting appears to work fine
* frequency mask trigger
    * updated to allow the mask and summed power arrays to be configured, either directly in the configuration file, or with a path to another file (such as that output by the above)
        * tested in file value arrays by setting in a file and calling write mask to ensure the values are in the output file
        * tested  from-file by modifying the above output file (so that the values differ), configuring with it as input, and the writing a new output to compare
    * added support for specifying thresholds to be measured in units of sigma of the noise, in addition to power (in dB, amplitude, etc.)
        * building a mask now must accumulate variance data as well as power data
        * tested by checking sigma mask matches data-mean + sigma_threshold * sqrt(data-variance)
        * mask file contains data-mean, data-variance, mask and mask2 if present
    * in two-level trigger-mode a second mask is created and stored; two masks can also be read in
        * mask sizes are compared after reading
        * tested via batch mode that fmt throws error and psyllid deactivates after reading in a mask from a file if sizes mismatch
        * mask sizes are compared to incoming data array when run is started
        * tested via batch mode that a missing mask or mismatching mask sizes results in an error when run is started; psyllid exits
* egg3-reader: support for "repeat-egg" boolean configuration option, if true, restarts reading the file from the first record upon reaching end of file
    * tested via batch mode, using two sequential start-run commands with duration set to 0 and the egg reader configured to read 100000 records (file has ~120k records). The second run repeated the egg file (debug prints showed it re-reading earlier record IDs) and prints of the output pkt_id showed that they continued to increase as expected.
* batch_executor: check return code of each action and exit if >= 100 (ie if an error occurred)
    * tested with valid config file and one with a syntax error to cause error, both behave as expected (ie the latter causes a crash).


Version: 1.5.0
~~~~~~~~~~~~~~~~~

Release Date: May 8, 2018
'''''''''''''''''''''''''

New Features:
'''''''''''''

* batch_executor receives the reply message's payload and return code; each action happens after the prior one returns (which may not be the conclusion of the action, just like any dripline request)
* frequency mask trigger
    * updated to also output the summed power data in addition to the spline fit used to define the frequency mask. This goes into a second array in the same output file
        * tested using the egg reader and confirming qualitatively that the mask follows the shape of the accumulated power (after normalizing by the number of accumulated points and the mask's offset)
* Dripline-cpp updated to v1.6.0
* CMake option added to allow disabling the FPA on linux builds (useful for batch mode execution without root access).
* midge updated to v3.5.3 (updates scarab to v1.6.0)
* server_config now only sets the default authentication file path after checking that the path exists
    * tested via docker batch execution with and without the auth file present; detection and setting appears to work fine

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
