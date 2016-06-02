==========
Server API
==========

The server communicates with `dripline-standard <https://github.com/project8/hardware/wiki/Wire-Protocol>`_ AMQP messages.

The API documentation below is organized by message type.

The routing key should consist of the server queue name, followed by the Routing Key Specifier (RKS): `[queue].[RKS]`.

For each message type, the API includes the allowed RKS, Payload options, and contents of the Reply Payload.


Lockout
=======

The server can be locked out to require a key for some types of messages.  
It obeys the lockout behavior specified by the dripline wire protocol.  
The server keeps a lockout tag that holds the information about who enabled the lock.  
Additionally, there is a ``OP_GET`` request ``is-locked`` that can be used to assess the state of the lock.


Message Types
=============

OP_RUN
^^^^^^

The `run` message type is used to start an acquisition.

All `run` requests are lockable.

There are no Routing Key Specifiers for *run* requests.

*Payload*

- ``file=[filename (string)]`` -- *(not implemented yet)* *(optional)* Filename for the acquisition.
- ``description=[description (string)]`` -- *(not implemented yet)* *(optional)* Text description for the acquisition; saved in the file header.
- ``duration=[ms (unsigned int)]`` -- *(not implemented yet)* *(optional)* Duration of the run in ms.

*Reply Payload*

- ``[Acquisition Request]`` -- The full request for the acquisition.
- ``status-meaning`` -- A human-readable interpretation of the acquisition status.


OP_GET
^^^^^^

The `get` message is used to request the status of various components of the server.

No `get` requests are lockable.

Routing Key Specifiers
----------------------

The Routing Key Specifier (RKS) indicates the information that is being requested from the server.  Some specifiers have *Payload* options.  The RKS and *Payload* options are listed below, along with the relevant *Reply Payload*.

``daq-status``
--------------
Returns the current acquisition configuration.

*Reply Payload*

- ``status=[status (string)]`` -- human-readable status message
- ``status-value=[status code (unsigned int)]`` -- machine-redable status message

``node.[node name]``
--------------------
Returns the configuration of the node requested.

*Reply Payload*

- ``[Full node configuration]``

``node.[node name].[parameter]``
--------------------------------
Returns the configuration value requested from the node requested.

*Reply Payload*

- ``values=[[value]]`` -- Array with the value requested

``is-locked``
-------------
Returns whether or not the server is locked out.

*Reply Payload*

- ``is-locked=[true/false (bool)]``


IGNORE EVERYTHING BELOW THIS
============================

OP_SET
^^^^^^

The `set` message type is used to set a value to a parameter in the acquisition configuration.

All `set` requests are lockable.

Routing Key Specifiers
----------------------

The RKS for `set` commands is the name of the configuration parameter being set.  For example:

- ``devices.pxie.enabled``
- ``duration``

*Payload*

- ``value=[value (varied)]`` -- *(required)* Specify the value to which the run-configuration item should be set.  Any values valid in the JSON standard will work, including strings, numbers, and ``true`` or ``false`` for booleans.

*Reply Payload*

- ``[Acquisition configuration]`` -- the full acquisition configuration **after** the `set` operation


OP_CMD
^^^^^^

The `cmd` message type is used to run a variety of different command instructions.  The instruction can be specified either as a Routing Key Specifier or as the first element in the ``values`` array in the payload.  They are listed below in the Command Instructions section.

All `command` requests are lockable.

Global Payload Options
----------------------
- ``values=[[instruction]]`` -- *(optional)* If the command instruction is not included in the routing key specifier, it should be given in the payload as the first element of the ``values`` array.


Command Instructions
--------------------

``add.device``
--------------
Adds a device to the master run configuration. Requires that the device be specified as an instruction option (see Payload section below).

*Payload*

- ``[device type]=[device name (string)]`` -- *(required)* The device type should be one of the valid device types for the server being run.  The device name is the name that will be used to refer to this particular instance of the device in the server configuration.

*Reply Payload*

- ``[acquisition configuration]``

``remove.device.[device name]``
-------------------------------
Removes a device from the master run configuration.

*Reply Payload*

- ``[acquisition configuration]``

``replace-config``
------------------
Replaces the server's run configuration with the contents of the instruction options, or the JSON file specified in those options (see below).

*Payload*

- ``[acquisition configuration]`` -- *(required)* The full acquisition configuration should be specified.  The payload, whatever it is, is assumed to be the acquisition configuration.

*Reply Payload*

- ``[acquisition configuration]``

``lock``
--------
Requests that the server lockout be enabled. Nothing is done if already locked.

*Reply Payload*

- ``key=[UUID (string)]`` -- lockout key required for any lockable requests as long as the lock remains enabled
- ``tag=[lockout tag (object)]`` -- information about the client that requested that the lock be enabled

``unlock``
----------
Requests that the server lockout be disabled.

*Payload*

- ``force=[true (bool)]`` -- *(optional)* Disables the lockout without a key.

``ping``
--------
Check that the server receives requests and sends replies. No other action is taken.

``cancel-acq``
--------------
Remove an acquisition that is waiting to run from the queue.

*Reply Payload*

- ``[Acquisition Request]`` -- The full request for the acquisition.
- ``status-meaning`` -- A human-readable interpretation of the acquisition status.

``clear-queue``
---------------
Clear scheduled acquisitions from the queue.

``start-queue``
---------------
Start processing the requests in the queue (if the queue had previously been stopped)

``stop-queue``
--------------
Stop processing the requests in the queue (queue is left intact and acquisition in progress is not stopped; restart with ``start-queue``)

``stop-acq``
------------
Stop any acquisition that is currently running (queue processing will continue).

``stop-all``
------------
Stop processing the requests in the queue and any acquisition that is currently running. This is the same as issuing both a ``stop-queue`` command and a ``stop-acq`` command.

``quit-mantis``
---------------
Stop execution of the Mantis server.
