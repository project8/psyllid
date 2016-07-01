===================
Node Configurations
===================

**Note:** Classes that are registered with some name (e.g. nodes and presets) are specified below as: ``class`` (``name``).  Named instances (e.g. nodes in use in a preset) are specified below as: ``class-name`` (``instance-name``).

Nodes
=====

Producers
---------

* ``udp_receiver`` (``udp-receiver``)

  * Output 0: ``time_data``
  * Output 1: ``freq_data``

Transformers
------------

* ``event_builder`` (``event-builder``)
  
  * Input: ``trigger_flag``
  * Output: ``trigger_flag``

* ``frequency_mask_trigger`` (``frequency-mask-trigger``)

  * Input: ``freq_data``
  * Output: ``trigger_flag``

* ``single_valaue_trigger`` (``single-valaue-trigger``) [Obsolete]

  * Input: ``freq_data``
  * Output: ``trigger_flag``


Consumers
---------

* ``egg_writer`` (``egg_writer``)

  * Input 0: ``time_data``
  * Input 1: ``trigger_flag``

* ``streaming_writer``  (``streaming-writer``)

  * Input: ``time_data``

* ``terminator_freq_data`` (``term_freq_data``)

  * Input: ``freq_data``


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