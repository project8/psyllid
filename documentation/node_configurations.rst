===================
Node Configurations
===================

Nodes
=====

Producers
---------

``packet_receiver_fpa``
^^^^^^^^^^^^^^^^^^^^^^^
UDP packet receiver using fast-packet-acquisition (Linux only)

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
UDP packet receiver using standard socket networking

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

``egg3_reader``
^^^^^^^^^^^^^^^
Egg file reader based on the monarch3 library

* Type: ``egg3-reader``
* Configuration

  - "egg_path": string -- resolvable path tot he egg file from which to read time series data

* Output
  * 0: ``time_data``

____


Transformers
------------

``event_builder``
^^^^^^^^^^^^^^^^^
Considers triggered packets and accounts for pretrigger and skipped packets

* Type: ``event-builder``
* Configuration

  - "length": uint -- The size of the output buffer
  - "pretrigger": uint -- Number of packets to include in the event before the first triggered packet
  - "skip-tolerance": uint -- Number of untriggered packets to include in the event between two triggered packets

* Input

  * 0: ``trigger_flag``

* Output

  * 0: ``trigger_flag``

``frequency_mask_trigger``
^^^^^^^^^^^^^^^^^^^^^^^^^^
Packet trigger based on high power above a pre-calculated frequency mask

* Type: ``frequency-mask-trigger``
* Configuration

  - "length": uint -- The size of the output data buffer
  - "n-packets-for-mask": uint -- The number of spectra used to calculate the trigger mask
  - "threshold-ampl-snr": float -- The threshold SNR, given as an amplitude SNR
  - "threshold-power-snr": float -- The threshold SNR, given as a power SNR
  - "threshold-dB": float -- The threshold SNR, given as a dB factor

* Input

  * 0: ``freq_data``

* Output

  * 0: ``trigger_flag``

``frequency_transform``
^^^^^^^^^^^^^^^^^^^^^^^
Compute fourier transform of time dataa

* Type: ``frequency-transform``
* Configuration

  - "time-length": uint -- The size of the output time-data buffer
  - "freq-length": uint -- The size of the output frequency-data buffer
  - "fft-size": unsigned -- The length of the fft input/output array (each element is 2-component)
  - "start-paused": bool -- Whether to start execution paused and wait for an unpause command
  - "transform-flag": string -- FFTW flag to indicate how much optimization of the fftw_plan is desired
  - "use-wisdom": bool -- whether to use a plan from a wisdom file and save the plan to that file
  - "wisdom-filename": string -- if "use-wisdom" is true, resolvable path to the wisdom file

* Input

  * 0: ``time_data``

* Output

  * 0: ``time_data``
  * 1: ``freq_data``

``tf_roach_receiver``
^^^^^^^^^^^^^^^^^^^^^
Splits raw combined time-frequency stream into time and frequency streams

* Type: ``tf-roach-receiver``
* Configuration

  - "time-length": uint -- The size of the output time-data buffer
  - "freq-length": uint -- The size of the output frequency-data buffer
  - "udp-buffer-size": uint -- The number of bytes in the UDP memory buffer for a single packet; generally this shouldn't be changed and is specified by the ROACH configuration
  - "time-sync-tol": uint -- (currently unused) Tolerance for time synchronization between the ROACH and the server (seconds)
  - "start-paused": bool -- Whether to start execution paused and wait for an unpause command
  - "force-time-first": bool -- If true, when starting ignore f packets until the first t packet is received

* Input

  * 0: ``memory_block``

* Output

  * 0: ``time_data``
  * 1: ``freq_data``

____


Consumers
---------

``egg_writer``
^^^^^^^^^^^^^^
Writes triggered data to an egg file

* Type: ``egg-writer``
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
Writes streamed data to an egg file

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

``streaming_frequency_writer``
^^^^^^^^^^^^^^^^^^^^
Writes streamed frequency data to an egg file

* Type: ``streaming-frequency-writer``
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

  * 0: ``freq_data``

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

    * ``packet-receiver-socket`` (``prf``)
    * ``tf-roach-receiver`` (``tfrr``)
    * ``frequency-mask-trigger`` (``fmt``)
    * ``egg-writer`` (``ew``)

  * Connections

    * ``prf.out_0:tfrr.in_0``
    * ``tfrr.out_0:ew.in_0``
    * ``tfrr.out_1:fmt.in_0``
    * ``fmt.out_0:ew.in_1``
