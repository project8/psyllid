How Psyllid Works
=================

Goals and Organization
----------------------

The main goals of the Psyllid architecture are to:

1. be module and configurable at runtime,  
2. safely run components of the DAQ system in parallel, and
3. integrate into an existing Dripline controls system

The source directory of Psyllid includes the following subdirectories:

1. ``applications`` --- executables
2. ``control`` --- classes responsible for control of the DAQ system and the user interface
3. ``data`` --- data classes
4. ``daq`` --- classes responsible for the actual data acquisition
5. ``test`` --- test programs
6. ``utility`` --- utility classes

The basic setup of the classes and the interactions between them is shown here:

.. image:: https://docs.google.com/drawings/d/e/2PACX-1vSdQ4VH0VSHYImB88g6McTBz5XpdnJ00ziCuLqHu65EkzuUpmxUzEHc3up5M-FlMBH1X_p9-8dPj6r1/pub?w=960&h=960
  :width: 500
  :alt: Psyllid Diagram

The main components (most coming from the Sandfly package) and their responsibilities are:

* ``conductor`` (sandfly) --- creates and configures the main Psyllid components, and starts their threads;
* ``batch_executor`` (sandfly) --- submits pre-configured commands after the main Psyllid components have started up;
* ``request_receiver`` (sandfly) --- provides the dripline interface for user requests;
* ``stream_manager`` (sandfly) --- creates and configures nodes;
* ``run_control`` (sandfly) --- starts and stops runs, and peforms active configurtion operations;
* ``diptera` (midge)` --- owns and operates the DAQ nodes;
* ``message_relayer`` (sandfly) --- sends message to Slack via dripline;
* ``butterfly_house`` (psyllid) --- provides centralized interactions with the Monarch3 library, and deadtime-less file creation.


Data Acquisition
----------------

The core data-acquisition capabilities are built on the Midge framework.  The capabilities of the system are implemented in "nodes," each of which has a particular responsibility (e.g. one node receives packets off the network; another node determines if the data passes a trigger; another node writes data to disk).  Nodes are connected to one another by circular buffers of data objects. 

User Interface
--------------

On Startup
''''''''''

The user typically starts Psyllid with a YAML configuration file.  Several example configuration files can be found in the ``examples`` directory.  Most configuration files will include these top-level blocks:

1. ``dripline`` --- Dripline broker information;
2. ``batch-actions`` --- commands to be submitted automatically after the components of Psyllid have started up;
2. ``daq`` --- Information for the DAQ controller, such as how many files to prepare, maximum file size, and whether to activate the DAQ on startup;
3. ``streams`` -- Definitions of the streams to be used.

Configuration options can also be set/modified from the command line.

Example Command Line Usage
**************************

::

    > bin/psyllid -c my_config.yaml -b my.amqp.broker

Explanation:

* ``bin/psyllid`` is the Psyllid executable
* ``-c my_config.yaml`` specifies ``my_config.yaml`` as the file that will set most of the configuration values
* ``-b my.amqp.broker`` modifies this value in the configuration::

    dripline:
      broker: my.amqp.broker

For full usage documentation, use::

    > bin/psyllid -h


During Execution
''''''''''''''''

User interaction with Psyllid during execution is performed via the dripline protocol.   Psyllid uses the ``dripline-cpp`` library to enable that interface.  The ``request_receiver`` class (a ``dripline::hub``) ties dripline commands to particular functions in the ``stream_manager``, ``daq_control``, and ``conductor`` classes.  

See the :ref:`api-label` page for more information about the dripline API for Psyllid.
