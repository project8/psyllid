===================
Node Configurations
===================

Nodes
=====

Producers
---------

``packet_receiver_fpa``
^^^^^^^^^^^^^^^^^^^^^^^
A producer to receive UDP packets via the fast-packet-acquisition interface and write them as raw blocks of memory.
Works in Linux only. 
Parameter setting is not thread-safe. Executing is thread-safe.

* Type: ``packet-receiver-fpa``
* Configuration

  - "length": uint -- The size of the output buffer
  - "max-packet-size": uint -- Maximum number of bytes to be read for each packet; larger packets will be truncated
  - "port": uint -- UDP port to listen on for packets
  - "interface": string -- Name of the network interface to listen on for packets
  - "timeout-sec": uint -- Timeout (in seconds) while listening for incoming packets; listening for packets repeats after timeout
  - "n-blocks": uint -- Number of blocks in the mmap ring buffer
  - "block-size": uint -- Number of packets per block in the mmap ring buffer
  - "frame-size": uint -- Number of blocks per frame in the mmap ring buffer

* Output

  * 0: ``memory_block``

``packet_receiver_socket``
^^^^^^^^^^^^^^^^^^^^^^^^^^
A producer to receive UDP packets via the standard socket interface and write them as raw blocks of memory.
Parameter setting is not thread-safe.  Executing is thread-safe.

* Type: ``packet-receiver-socket``
* Configuration

  - "length": uint -- The size of the output buffer
  - "max-packet-size": uint -- Maximum number of bytes to be read for each packet; larger packets will be truncated
  - "port": uint -- UDP port to listen on for packets
  - "ip": string -- IP port to listen on for packets; must be in IPV4 numbers-and-dots notation (e.g. 127.0.0.1)
  - "timeout-sec": uint -- Timeout (in seconds) while listening for incoming packets; listening for packets repeats after timeout

* Output

  * 0: ``memory_block``

* ``udp_receiver`` (``udp-receiver``)

  * Output 0: ``time_data``
  * Output 1: ``freq_data``

____


Transformers
------------

``event_builder``
^^^^^^^^^^^^^^^^^
Keeps track of the state of the packet sequence (is-triggered, or not).

When going from untriggered to triggered, adds some number of pretrigger packets.
Includes a skip tolerance for untriggered packets between two triggered packets.

In untriggered state, the ``flag`` and the ``high_threshold`` variables of the trigger flags are checked.
Only if both are true the event builder switches state.
If ``f_n_triggers`` > 1 the next state is collecting_triggers, otherwise triggered.
In collecting_triggers state the event builder counts the incomming trigger flags with ``flag =  true``.
Only if this ``count == f_n_triggers`` the state is switched to triggered.

Events are built by switching some untriggered packets to triggered packets according to the pretrigger and skip-tolerance parameters.
Contiguous sequences of triggered packets constitute events.

Parameter setting is not thread-safe.  Executing is thread-safe.

The cofigurable value ``time-lengt`` in the tf_roach_receiver must be set to a value greater than ``pretrigge`` and ``skip-tolerance`` (+5 is advised).
Otherwise the time domain buffer gets filled and blocks further packet processing.

* Type: ``event-builder``
* Configuration

  - "length": uint -- The size of the output buffer
  - "pretrigger": uint -- Number of packets to include in the event before the first triggered packet
  - "skip-tolerance": uint -- Number of untriggered packets to include in the event between two triggered packets
  - "n-triggers": uint -- Number of trigger flags with flag == true required before switching to triggered state

* Input

  * 0: ``trigger_flag``

* Output

  * 0: ``trigger_flag``

``frequency_mask_trigger``
^^^^^^^^^^^^^^^^^^^^^^^^^^
The FMT has two modes of operation: updating the mask, and triggering.

When switched to the "updating" mode, the existing mask is erased, and the subsequent spectra that are passed to the FMT are used to calculate the new mask. The number of spectra used for the mask is configurable.  Those spectra are summed together as they arrive. Once the appropriate number of spectra have been used, the average value is calculated, and the mask is multiplied by the threshold SNR (assuming the data are amplitude values, not power).

In triggering mode, each arriving spectrum is compared to the mask bin-by-bin.  If a bin crosses the threshold, the spectrum passes the trigger and the bin-by-bin comparison is stopped.

It is possible to set a second threshold (``threshold-power-snr-high``).
In this case a second mask is calculated for this threshold and the incoming spectra are compared to both masks.
The output trigger flag has an additional variable ``high_threshold`` which is set true if the higher threshold led to a trigger. This variable is always set to true if only one trigger level is used.

The mask can be written to a JSON file via the write_mask() function.  The format for the file is:
     {
     
         "timestamp": "[timestamp]",
         "n-packets": [number of packets averaged],
         "mask": [value_0, value_1, . . . .]
         
     }

Parameter setting is not thread-safe.  Executing (including switching modes) is thread-safe.

* Type: ``frequency-mask-trigger``
* Configuration

  - "length": uint -- The size of the output data buffer
  - "n-packets-for-mask": uint -- The number of spectra used to calculate the trigger mask
  - "threshold-ampl-snr": float -- The threshold SNR, given as an amplitude SNR
  - "threshold-power-snr": float -- The threshold SNR, given as a power SNR
  - "threshold-power-snr-high": float -- A second SNR threshold, given as power SNR
  - "threshold-dB": float -- The threshold SNR, given as a dB factor
  - "trigger-mode": string -- The trigger mode, can be set to "single-level-trigger" or "two-level-trigger"
  - "n-spline-points": uint -- The number of points to have in the spline fit for the trigger mask

* Input

  * 0: ``freq_data``

* Output

  * 0: ``trigger_flag``

``tf_roach_receiver``
^^^^^^^^^^^^^^^^^^^^^
Splits raw combined time-frequency stream into time and frequency streams.
Parameter setting is not thread-safe.  Executing is thread-safe.

* Type: ``tf-roach-receiver``
* Configuration

  - "time-length": uint -- The size of the output time-data buffer
  - "freq-length": uint -- The size of the output frequency-data buffer
  - "udp-buffer-size": uint -- The number of bytes in the UDP memory buffer for a single packet; generally this shouldn't be changed and is specified by the ROACH configuration
  - "time-sync-tol": uint -- (currently unused) Tolerance for time synchronization between the ROACH and the server (seconds)
  - "start-paused": bool -- Whether to start execution paused and wait for an unpause command

* Input

  * 0: ``memory_block``

* Output

  * 0: ``time_data``
  * 1: ``freq_data``

____


Consumers
---------

``triggered_writer``
^^^^^^^^^^^^^^^^^^^^
Writes triggered data to an egg file.
Parameter setting is not thread-safe.  Executing is thread-safe.

* Type: ``triggered-writer``
* Configuration

  - "file-size-limit-mb": uint -- Not used currently
  - "device": node -- digitizer parameters

    - "bit-depth": uint -- bit depth of each sample
    - "data-type-size": uint -- number of bytes in each sample (or component of a sample for sample-size > 1)
    - "sample-size": uint -- number of components in each sample (1 for real sampling; 2 for IQ sampling)
    - "record-size": uint -- number of samples in each record
    - "acq-rate": uint -- acquisition rate in MHz
    - "v-offset": double -- voltage offset for ADC calibration
    - "v-range": double -- voltage range for ADC calibration
  - "center-freq": double -- the center frequency of the data being digitized
  - "freq-range": double -- the frequency window (bandwidth) of the data being digitized

* Input

  * 0: ``time_data``
  * 1: ``trigger_flag``

``roach_freq_monitor``
^^^^^^^^^^^^^^^^^^^^^^
Checks for missing frequency packets

* Type: ``roach-freq-monitor``
* Configuration (none)
* Input

  * 0: ``freq_data``

``roach_time_monitor``
^^^^^^^^^^^^^^^^^^^^^^
Checks for missing time packets

* Type: ``roach-time-monitor``
* Configuration (none)
* Input

  * 0: ``time_data``

``streaming_writer``
^^^^^^^^^^^^^^^^^^^^
Writes streamed data to an egg file.
Parameter setting is not thread-safe.  Executing is thread-safe.

* Type: ``streaming-writer``
* Configuration

  - "file-size-limit-mb": uint -- Not used currently
  - "device": node -- digitizer parameters

    - "bit-depth": uint -- bit depth of each sample
    - "data-type-size": uint -- number of bytes in each sample (or component of a sample for sample-size > 1)
    - "sample-size": uint -- number of components in each sample (1 for real sampling; 2 for IQ sampling)
    - "record-size": uint -- number of samples in each record
    - "acq-rate": uint -- acquisition rate in MHz
    - "v-offset": double -- voltage offset for ADC calibration
    - "v-range": double -- voltage range for ADC calibration

  - "center-freq": double -- the center frequency of the data being digitized
  - "freq-range": double -- the frequency window (bandwidth) of the data being digitized

* Input

  * 0: ``time_data``

``terminator_freq``
^^^^^^^^^^^^^^^^^^^
Does nothing with frequency data

* Type: ``terminator-freq``
* Configuration (none)
* Input

  * 0: ``freq_data``

``terminator_time``
^^^^^^^^^^^^^^^^^^^
Does nothing with time data

* Type: ``terminator-time``
* Configuration (none)
* Input

  * 0: ``time_data``


Stream Presets
==============

* ``streaming_1ch`` (``str-1ch``)

  * Nodes

    * ``packet-receiver-socket`` (``prs``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``streaming-writer`` (``strw``)
    * ``term-freq-data`` (``term``)

  * Connections

    * ``prs.out_0:tfrr.in_0``
    * ``tfrr.out_0:strw.in_0``
    * ``tfrr.out_1:term.in_0``


* ``streaming_1ch_fpa`` (``str-1ch-fpa``)

  * Nodes

    * ``packet-receiver-socket`` (``prf``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``streaming-writer`` (``strw``)
    * ``term-freq-data`` (``term``)

  * Connections

    * ``prf.out_0:tfrr.in_0``
    * ``tfrr.out_0:strw.in_0``
    * ``tfrr.out_1:term.in_0``


* ``fmask_trigger_1ch`` (``fmask-1ch``)

  * Nodes

    * ``packet-receiver-socket`` (``prs``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``frequency-mask-trigger`` (``fmt``)
    * ``triggered-writer`` (``trw``)

  * Connections

    * ``prs.out_0:tfrr.in_0``
    * ``tfrr.out_0:trw.in_0``
    * ``tfrr.out_1:fmt.in_0``
    * ``fmt.out_0:trw.in_1``


* ``fmask_trigger_1ch_fpa`` (``fmask-1ch-fpa``)

  * Nodes

    * ``packet-receiver-fpa`` (``prf``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``frequency-mask-trigger`` (``fmt``)
    * ``triggered-writer`` (``trw``)

  * Connections

    * ``prf.out_0:tfrr.in_0``
    * ``tfrr.out_0:trw.in_0``
    * ``tfrr.out_1:fmt.in_0``
    * ``fmt.out_0:trw.in_1``


* ``event_builder_1ch`` (``events-1ch``)

  * Nodes

    * ``packet-receiver-socket`` (``prs``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``frequency-mask-trigger`` (``fmt``)
    * ``event-builder`` (``eb``)
    * ``triggered-writer`` (``trw``)

  * Connections

    * ``prs.out_0:tfrr.in_0``
    * ``tfrr.out_0:trw.in_0``
    * ``tfrr.out_1:fmt.in_0``
    * ``fmt.out_0:eb.in_0``
    * ``eb.out_0:trw.in_1``


* ``event_builder_1ch_fpa`` (``events-1ch-fpa``)

  * Nodes

    * ``packet-receiver-fpa`` (``prf``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``frequency-mask-trigger`` (``fmt``)
    * ``event-builder`` (``eb``)
    * ``triggered-writer`` (``trw``)

  * Connections

    * ``prf.out_0:tfrr.in_0``
    * ``tfrr.out_0:trw.in_0``
    * ``tfrr.out_1:fmt.in_0``
    * ``fmt.out_0:eb.in_0``
    * ``eb.out_0:trw.in_1``
