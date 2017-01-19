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
* Output

  * 0: ``memory_block``

``packet_receiver_socket``
^^^^^^^^^^^^^^^^^^^^^^^^^^
UDP packet receiver using standard socket networking

* Type: ``packet-receiver-socket``
* Configuration
* Output

  * 0: ``memory_block``

* ``udp_receiver`` (``udp-receiver``)

  * Output 0: ``time_data``
  * Output 1: ``freq_data``

Transformers
------------

``event_builder``
^^^^^^^^^^^^^^^^^
Considers triggered packets and accounts for pretrigger and skipped packets

* Type: ``event-builder``
* Configuration
* Input

  * 0: ``trigger_flag``

* Output

  * 0: ``trigger_flag``

``frequency_mask_trigger``
^^^^^^^^^^^^^^^^^^^^^^^^^^
Packet trigger based on high power above a pre-calculated frequency mask

* Type: ``frequency-mask-trigger``
* Configuration
* Input

  * 0: ``freq_data``

* Output

  * 0: ``trigger_flag``

``tf_roach_receiver``
^^^^^^^^^^^^^^^^^^^^^
Splits raw combined time-frequency stream into time and frequency streams

* Type: ``tf-roach-receiver``
* Configuration
* Input

  * 0: ``memory_block``

* Output

  * 0: ``time_data``
  * 1: ``freq_data``


Consumers
---------

``egg_writer``
^^^^^^^^^^^^^^
Writes triggered data to an egg file

* Type: ``egg-writer``
* Configuration
* Input

  * 0: ``time_data``
  * 1: ``trigger_flag``

``roach_freq_monitor``
^^^^^^^^^^^^^^^^^^^^^^
Checks for missing frequency packets

* Type: ``roach-freq-monitor``
* Configuration
* Input

  * 0: ``freq_data``

``roach_time_monitor``
^^^^^^^^^^^^^^^^^^^^^^
Checks for missing time packets

* Type: ``roach-time-monitor``
* Configuration
* Input

  * 0: ``time_data``

``streaming_writer``
^^^^^^^^^^^^^^^^^^^^
Writes streamed data to an egg file

* Type: ``streaming-writer``
* Configuration
* Input

  * 0: ``time_data``

``terminator_freq``
^^^^^^^^^^^^^^^^^^^
Does nothing with frequency data

* Type: ``terminator-freq``
* Configuration

  * (none)

* Input

  * 0: ``freq_data``

``terminator_time``
^^^^^^^^^^^^^^^^^^^
Does nothing with time data

* Type: ``terminator-time``
* Configuration

  * (none)

* Input

  * 0: ``time_data``


Preset Configurations
=====================

* ``roach_config`` (``roach``)
  
  * Nodes

    * ``udp-receiver`` (``udpr``)

  * Connections

* ``streaming_1ch`` (``str-1ch``)

  * Nodes

    * ``udp-receiver`` (``udpr``)
    * ``streaming-writer`` (``strw``)
    * ``term-freq-data`` (``term``)

  * Connections

    * ``udpr.out_0:strw.in_0``
    * ``udpr.out_1:term.in_0``