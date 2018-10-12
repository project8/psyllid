===================
Getting started
===================

Introduction
-------------

Psyllid is a data acquisition package developed by the Project 8 collaboration and it's purpose is to receive UDP packets via a port, unpack and analyze them in real-time and write data to egg-files_ using Monarch_.


Currently, the Project 8 experiment uses a ROACH2_ board to continuosly digitize and packetize a mixed down RF-signal (for more details on the packet structure and content see RoachPackets_). The ROACH2 streams UDP packets at a rate of 24kHz via 3 10GBe connections to a server. In order to keep up with such a high rate of incoming packets, Psyllid is written multi-threaded and the packet processing and data analysis is done in parallel by various nodes_.
By running psyllid with different `node configurations`_, psyllid can be used to serve different purposes. It can for example continously write the entire incoming data to files or examine the packet content and decide based on some pre-defined criteria whether or not to write out the content.




Running psyllid
-----------------


Running requirements
^^^^^^^^^^^^^^^^^^^^

- rabbit broker
- broker authentication file


Start psyllid
^^^^^^^^^^^^^^

The psyllid executable can be found in */path_to_build/bin/*. Running the exectuabel ``/path_to_build/bin/psyllid`` will result in a psyllid instance being created. Once started, psyllid will initialize it's control classes, connect to the broker and start the pre-configured daq nodes. If psyllid cannot connect to a broker, it will exit.


Configuration file
^^^^^^^^^^^^^^^^^^^

By using the command line option *-c* and specifying the path to a configuration file in YAML-format
::

 bin/psyllid -c config=/path-to-config/config.yaml

psyllid's node connections and node configurations can be set during start-up.

This is how a configuration file for running psyllid in streaming mode could look like:

::

    amqp:
        broker: localhost
        queue: psyllid

    post-to-slack: false

    daq:
        activate-at-startup: true
        n-files: 1
        max-file-size-mb: 500

    streams:
        ch0:
            preset: str-1ch-fpa

            device:
                n-channels: 1
                bit-depth: 8
                data-type-size: 1
                sample-size: 2
                record-size: 4096
                acq-rate: 100 # MHz
                v-offset: 0.0
                v-range: 0.5

            prf:
                length: 10
                port: 23530
                interface: eth1
                n-blocks: 64
                block-size: 4194304
                frame-size: 2048

            strw:
    file-num: 0

In this example the broker is running on localhost and a queue will be set up with the name `psyllid`. If no path to the broker-authentication file is provided, Psyllid will look for *~/.project8_authentications.json*.
The *post-to-slack* option, allows psyllid to post some messages (like "psyllid is starting up" or "Run is commencing") to a slack channel. Under *daq*, some general settings are configured. For example the maximum file size is set to 500 MB which means that psyllid will continue to write in a new egg file once the size of the last egg file has reached this size.
In the *streams* section one stream with the name *ch0* is configured. In this stream the nodes are connected according to the preset *str-1ch-fpa*. The configurations of the individual nodes from this stream is specified withing the *stream* block. In theory, psyllid can be run with multiple streams, each of which could be set up with a different node configuration. In practice though, psyllid is mostly used with only one stream set up and in case data should be read from multiple ports, multiple instances of psyllid are run in parallel.


Node connections and presets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Which nodes will be set up and how they will be connected can be specified either by using a preset as in the example above or manually in the configuration file. This is how the same node configuration as above could be achieved manually:

::

    preset:
        type: reader-stream
        nodes:
          - type: packet-receiver-fpa
            name: prf
          - type: tf-roach-receiver
            name: tfrr
          - type: streaming-writer
            name: strw
          - type: term-freq-data
            name: term
        connections:
          - "prf.out_0:tfrr.in_0"
          - "tfrr.out_0:strw.in_0"
          - "tfrr.out_1:term.in_0"

The available presets can be found here_.

.. _here: https://psyllid.readthedocs.io/en/latest/node_configurations.html#stream-presets



on-startup
^^^^^^^^^^^^^

A *on-startup* block can be added to the configuration file with a list of requests that psyllid will process and act upon right after starting-up. This is especially convenient when using psyllid to read data from a file instead of from a port (see `egg-reader`_ for more on this). In this case psyllid will only start the the *run-server* for receiving commands from a user after executing all the on-startup commands. Here is an example for a *on-startup* command block:

::

    amqp:
        broker: rabbit_broker
        queue: psyllid
        make-connection: false

    on-startup:
      - type: get
        rks: daq-status
        payload: {}
        sleep-for: 0
      - type: wait-for
        rks: daq-status
        payload: {}
        sleep-for: 1000
      - type: wait-for
        rks: daq-status
        payload: {}
        sleep-for: 100

In this example, the psyllid will not try to connect to the broker and as a result it will exit after processing the *on-startup* requests. The first 




Node configurations
---------------------

Nodes are held and connected by Midge_.

.. _egg-reader:

Batch mode
^^^^^^^^^^^^^^

Instead of reading data packets that were received via a port, psyllid has the option to read the content from an egg file.
By using the *egg-reader* node and configuring this node with the path to the file that it should read from, psyllid will perfrom the exact same operations as it would in normal operation on the content of a file.
By defining a list of start-up commands in the configuration file, psyllid will perform all of them and then exit once the preocessing of the file content was completed.

Here is an example for an egg-reader configuraion:

::

    e3r:
        length: 1000
        egg-path: /a_test_file.egg
        read-n-records: 0



.. _ROACH2: https://casper.berkeley.edu/wiki/ROACH-2_Revision_2
.. _RoachPackets: https://psyllid.readthedocs.io/en/latest/roach_packets.html
.. _nodes: https://psyllid.readthedocs.io/en/latest/node_configurations.html
.. _node configurations: https://psyllid.readthedocs.io/en/latest/node_configurations.html
.. _egg-files: https://monarch.readthedocs.io/en/stable/EggStandard.v3.2.0.html
.. _Monarch: https://monarch.readthedocs.io/en/stable/index.html
.. _Midge: https://midge.readthedocs.io/en/latest/



