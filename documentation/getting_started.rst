===================
Getting started
===================

Introduction
-------------

Psyllid is a data acquisition package developed by the Project 9 collaboration and it's purpose is to receive UDP packets via a port, unpack and analyze them in real-time and write data to egg-files_ using Monarch_.


Currently, the Project 8 experiment uses a ROACH2_ board to continuesly digitize and packetize a mixed down RF-signal (for more details on the packet structure and content see RoachPackets_). The ROACH2 streams UDP packets at a rate of 24kHz via 3 channels to a server. In order to keep up with an incoming packet rate of 24kHz, Psyllid is written multi-threaded and the packet processing and data analysis is done in parallel by various nodes_.
By running psyllid with different node-condigurations, psyllid can be used to serve different purposes. It can for example continously write the entire incoming data to files or examine the packet content and decide based on some pre-defined criteria whether or not to write out the conten.




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

Fore more details on node configurations see the `node configurations`_ section.

Batch mode
^^^^^^^^^^^^^^

Instead of reading data packets that were received via a port, psyllid has the option to read the content from an egg file.
By using the *egg-reader* node and configuring this node with the path to the file that it should read from, psyllid will perfrom the exact same operations as it would in normal operation on the content of a file.
By defining a list of start-up commands in the configuration file, psyllid will 



Node configurations
---------------------

Nodes are held and connected by Midge_.


.. _ROACH2: https://casper.berkeley.edu/wiki/ROACH-2_Revision_2
.. _RoachPackets: https://psyllid.readthedocs.io/en/latest/roach_packets.html
.. _nodes: https://psyllid.readthedocs.io/en/latest/node_configurations.html
.. _node configurations: https://psyllid.readthedocs.io/en/latest/node_configurations.html
.. _egg-files: https://monarch.readthedocs.io/en/stable/EggStandard.v3.2.0.html
.. _Monarch: https://monarch.readthedocs.io/en/stable/index.html
.. _Midge: https://midge.readthedocs.io/en/latest/



