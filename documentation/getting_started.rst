===================
Getting started
===================

Introduction
-------------

Psyllid is a data acquisition package developed by the Project 8 collaboration and it's purpose is to receive UDP packets via a port, unpack and analyze them in real-time and write data to egg-files_ using Monarch_.


Currently, the Project 8 experiment uses a ROACH2_ board to continuously digitize and packetize a mixed down RF-signal (for more details on the packet structure and content see RoachPackets_). The ROACH2 streams UDP packets at a rate of 24kHz via three 10GbE connections to a server. In order to keep up with such a high rate of incoming packets, Psyllid is written multi-threaded and the packet processing and data analysis is done in parallel by various nodes_.
By running psyllid with different `node configurations`_, psyllid can be used to serve different purposes. It can for example continuously write the entire incoming data to files or examine the packet content and decide based on some pre-defined criteria whether or not to write out the content.


Psyllid's internal structure basically consists of 2 layers: the *control* layer and the *daq* layer.
The classes in the *control* layer control the *daq* layer and allow a user to interact with psyllid. Dripline_ commands are received by the *run-server* and are forwarded to their destination node-binding. Upon request, an instance of the *daq-control* class tells Midge_ to deactivate or activate the daq-nodes and to start or stop a run. 
Psyllid knows different states: deactivated, activating, activated, running, canceled, do_restart, done and error.
Data is only being taken in *running* state.

For more information read the sections `How psyllid works`_ and `DAQ Status`_.


**A note on data rates and starting runs**


Psyllid will only go to *running* state if told by the user via a `on-startup`_ command or by sending the *start-run* dripline_ request. When a writer node is configured psyllid will write data to files in this state.
In `streaming-mode` the data rate is about 200MB/s. So be careful with run durations. The default run-duration is 1s. If you start a run without specifying a run duration or a filepath it will result in a 200MB file being written to the directory in which you ran the psyllid executable.
In all other states no data is being taken/written. So as long as you don't tell psyllid to start-running you don't need to worry about accidentally taking data without noticing.
And in case of emergency: *Control-C*




Running psyllid
-----------------

For installation instructions go here_

.. _here: https://psyllid.readthedocs.io/en/latest/installation.html

Running psyllid does not mean start a psyllid run. Psyllid starts in deactivated or activated status.

Running requirements
^^^^^^^^^^^^^^^^^^^^



- rabbit broker
- broker authentication file


Start psyllid
^^^^^^^^^^^^^^

The psyllid executable can be found in */path_to_build/bin/*. Running the executable ``/path_to_build/bin/psyllid`` will result in a psyllid instance being created. Once started, psyllid will initialize its control classes, connect to the broker and start the pre-configured daq nodes. If psyllid cannot connect to a broker, it will exit.


Permissions
^^^^^^^^^^^^

Generally, psyllid can be run without sudo permissions. However, they are required for using the *packet-receiver-fpa*. This node can shortcut the port and is therefore faster at reading the incoming packets. If you don't have sudo permissions you can only use the presets_ using the *packet-receiver-socket* node instead which cannot keep up with the ROACH2 packet rate.


Configuration files
---------------------

By using the command line option *-c* and specifying the path to a configuration file in YAML-format
::

 bin/psyllid config=/path-to-config/config.yaml

psyllid will be set up according to whatever is specified in the file and overwrite its default settings. Here is how a configuration file for running psyllid in streaming mode could look like:

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

In this example the broker is running on localhost and a queue will be set up with the name `psyllid`. If no path to the broker-authentication file is provided, psyllid will look for *~/.project8_authentications.json*.
The *post-to-slack* option, allows psyllid to post some messages (like "psyllid is starting up" or "Run is commencing") to a slack channel. Under *daq*, some general settings are configured. For example the maximum file size is set to 500 MB which means that psyllid will continue to write to a new egg file once the size of the last egg file has reached this size.
In the *streams* section one stream with the name *ch0* is configured. In this stream the nodes are connected according to the preset *str-1ch-fpa*. The configurations of the individual nodes from this stream must be specified within the *stream* block. Here the *prf* (packet-receiver-fpa) is the node that receives and distributes the incoming packets. It is configured with the interface name and the port to which the ROACH2 is streaming the packets to. The *length* in node configurations always refers to a buffer size. Generally, the buffers of nodes should be larger than the buffer sizes downstream, to prevent that the data processing gets blocked if one of the nodes is falling behind.

In theory, psyllid can be run with multiple streams, each of which could be set up with a different node configuration. In practice though, psyllid is mostly used with only one stream set up and in case data should be read from multiple ports, multiple instances of psyllid are run in parallel.


.. _presets:

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

The `available presets`_ can be found in `node configurations`_.

If you want to use psyllid to process ROACH2 packets and write all the content to files use the *str-1ch-fpa* preset.
If you want to take triggered data use *events-1ch-fpa*.


.. _on-startup:

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

In this example, the psyllid instance will not try to connect to the broker and as a result it will exit after processing the *on-startup* requests.


Interacting with psyllid
-------------------------

As mentioned a few times above, it is possible to send dripline_ requests via a rabbitmq broker to a running psyllid instance. There is a detailed list of which requests can be received and processed in `Psyllid API`_.

Assuming you have dragonfly_ installed, here are some examples for how to interact with psyllid from the command line:


* If you have a psyllid instance running (and it was configured to have "psyllid" as queue name), you can for example send a request to ask what state psyllid is in by running:
  ::

    dragonfly get psyllid.daq-status -b rabbit_broker

* Deactivate and activate psyllid:
  ::

    dragonfly cmd psyllid.activate-daq -b rabbit_broker

    dragonfly cmd psyllid.deactivate-daq -b rabbit_broker


* Make psyllid exit:
  ::

    dragonfly cmd psyllid.quit-psyllid -b rabbit_broker

* To start a 500ms run:
  ::

    dragonfly cmd psyllid.start-run duration=500 filename=a_test.egg -b rabbit_broker


* Node configurations can also be changed by sending the relevant request.
  If you are running psyllid using the *event_builder_1ch_fpa* preset you can set the snr trigger level of the frequency mask trigger node to 20 with:
  ::

    dragonfly set psyllid.active-config.ch0.fmt.threshold-power-snr 20 -b rabbit_broker

  Check the threshold setting with:
  ::

    dragonfly get psyllid.active-config.ch0.fmt.threshold-power-snr -b rabbit_broker


* Changing the buffer size of a node at run time, will only re applied after psyllid reactivates (because buffers are initialized when nodes are activated). 
  ::

    dragonfly cmd psyllid.reactivate-daq -b rabbit_broker



.. _egg-reader:

Egg reader
-------------

Instead of reading data packets that were received via a port, psyllid has the option to read the content from an egg file.
By using the *egg-reader* node and configuring this node with the path to the file that it should read from, psyllid will perfrom the exact same operations as it would in normal operation on the content of a file.
By defining a list of start-up commands in the configuration file, psyllid will perform all of them and then exit once the processing of the file content was completed.

Here is an example for an egg-reader configuration:

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
.. _Dripline: https://dripline.readthedocs.io/en/latest/
.. _available presets: https://psyllid.readthedocs.io/en/latest/node_configurations.html#stream-presets
.. _how psyllid works: https://psyllid.readthedocs.io/en/latest/how_psyllid_works.html
.. _Psyllid API: https://psyllid.readthedocs.io/en/latest/api.html
.. _DAQ Status: https://psyllid.readthedocs.io/en/latest/status_definitions.html
.. _dragonfly: https://p8-dragonfly.readthedocs.io/en/stable/

