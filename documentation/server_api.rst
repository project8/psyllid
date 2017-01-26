##########
Server API
##########

The server communicates with `dripline-standard <https://github.com/project8/dripline>`_ AMQP messages.

The API documentation below is organized by message type.

The routing key should consist of the server queue name, followed by the Routing Key Specifier (RKS): `[queue].[RKS]`.

For each message type, the API includes the allowed RKS, Payload options, and contents of the Reply Payload.


Dripline API
============

OP_GET
^^^^^^

``is-locked``
-------------
Returns whether or not the server is locked out.

*Reply Payload*

- ``is-locked=[true/false (bool)]``


OP_CMD
^^^^^^

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


Psyllid API
===========

OP_RUN
^^^^^^

The `run` message type is used to start a run.

All `run` requests are lockable.

There are no Routing Key Specifiers for *run* requests.

*Payload*

- ``file: [filename (string)]`` -- *(not implemented yet)* *(optional)* Filename for the acquisition.
- ``description: [description (string)]`` -- *(not implemented yet)* *(optional)* Text description for the acquisition; saved in the file header.
- ``duration: [ms (unsigned int)]`` -- *(not implemented yet)* *(optional)* Duration of the run in ms.


OP_GET
^^^^^^

The `get` message is used to request information from the server.

No `get` requests are lockable.

``daq-status``
--------------
Returns the current acquisition configuration.

*Reply Payload*

- ``status: [status (string)]`` -- human-readable status message
- ``status-value: [status code (unsigned int)]`` -- machine-redable status message

``node-config.[stream].[node]``
-------------------------------
Returns the configuration of the node requested.

*Reply Payload*

- ``[Full node configuration]``

``node-config.[stream].[node].[parameter]``
-------------------------------------------
Returns the configuration value requested from the node requested.

*Reply Payload*

- ``[parameter name]: [value]`` -- Parameter name and value

``filename``
------------
Returns the filename that will be written to.

*Reply Payload*

- ``values: [[filename (string)]]`` -- Filename as the first element of the ``values`` array

``description``
---------------
Returns the description that will be written to the file header.

*Reply Payload*

- ``values: [[description (string)]]`` -- Description as the first element of the ``values`` array

``duration``
------------
Returns the run duration (in ms).

*Reply Payload*

- ``values: [[duration (unsigned int)]]`` -- Duration in ms as the first element of the ``values`` array


OP_SET
^^^^^^

The `set` message type is used to set a value to a parameter in the configuration.

All `set` requests are lockable.

``node-config.[stream].[node]``
-------------------------------
Configures one or more parameters within a node.  Takes effect next time the DAQ is activated.

*Payload*

- ``[node configuration (dictionary)]`` -- Parameters to set in the node

*Reply Payload*

- ``[the parameters that were set (dictionary)]`` -- Parameter name:value pairs that were set

``node-config.[stream].[node].[parameter]``
-------------------------------------------
Configure a single parameter in a node.  Takes effect next time the DAQ is activated.

*Payload*

- ``values: [[value]]`` -- Parameter value to be set as the first element of the ``values`` array.

``filename``
------------
Sets the filename (relative or absolute) that will be written to. Takes effect for the next run.

*Payload*

- ``values: [[filename (string)]]`` -- Filename

``description``
---------------
Sets the description that will be written to the file header. Takes effect for the next run.

*Payload*

- ``values: [[description (string)]]`` -- Description

*Reply Payload*

- ``[the parameter that was set as a dictionary]`` -- Parameter name:value pair that was set

``duration``
------------
Sets the run duration in ms. Takes effect for the next run.

*Payload*

- ``values: [[duration (unsigned int)]]`` -- Duration in ms


OP_CMD
^^^^^^

The `cmd` message type is used to run a variety of different command instructions.

All `command` requests are lockable.

``add-stream``
--------------
Adds a stream to the DAQ configuration.  Takes effect next time the DAQ is activated.

*Payload*

- ``name: [stream name (string)]`` -- Unique name for the stream.
- ``config: [stream configuration (dictionary)]`` -- Configuration for the stream

``remove-stream``
-----------------
Remove a stream from the DAQ configuration.  Takes effect next time the DAQ is activated.

*Payload*

- ``values: [[stream name (string)]]`` -- Name of the stream to remove as the first element of the ``values`` array

``stop-run``
------------
Stop a run that's currently going on.

``start-run``
-------------
Same as the OP_RUN command above.

``activate-daq``
----------------
Put the DAQ in its actiavated state to be ready to take data.  Psyllid must be in its deactivated state before this call.

``reactivate-daq``
------------------
Deactivate, then reactivate the DAQ; it will end in its activated state, ready to take data.  Psyllid must be in its activated state before this call.

``deactivate-daq``
------------------
Put in its deactivated state, in which it is not immediately ready to take data.  Psyllid must be in its activated state before this call.

``quit-psyllid``
----------------
Instruct the Psyllid executable to exit.
