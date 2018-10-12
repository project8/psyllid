..server-api-label:

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

``set_condition``
-----------------
Cause the server to move to some defined state as quickly as possible; Psyllid implements conditions 10 and 12, both of which stop any ongoing run.
_Note:_ set_condition is expected to be a broadcast command (routing key target is `broadcast` not `<psyllid-queue>`).


*Payload*

- ``values=[condition (int)]`` -- the integer condition


Psyllid API
===========

OP_RUN
^^^^^^

The `run` message type is used to start a run.

All `run` requests are lockable.

There are no Routing Key Specifiers for *run* requests.

*Payload*

- ``filename: [filename (string)]`` -- *(optional)* Filename for ``file_number`` 0.
- ``filenames: [array of filenames (string)]`` -- *(optional)* Filenames for all files specified. Overrides ``filename``.
- ``description: [description (string)]`` -- *(optional)* Text description for ``file_number`` 0; saved in the file header.
- ``descriptions: [array of descriptions (string)]`` -- *(optional)* Text descriptions for all files specified.  Overrides ``description``.
- ``duration: [ms (unsigned int)]`` -- *(optional)* Duration of the run in ms.


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

``active-config.[stream].[node]``
-------------------------------
Returns the configuration of the active DAQ node requested.

*Reply Payload*

- ``[Full node configuration]``

``active-config.[stream].[node].[parameter]``
-------------------------------------------
Returns the configuration value requested from the active DAQ node requested.  
Please note that this action will not necessarily return the value in use (e.g. if a parameter that is only used once during initialization has been changed since then), and is not necessarily thread-safe.

*Reply Payload*

- ``[parameter name]: [value]`` -- Parameter name and value

``stream-list``
---------------
Returns a list of all streams in the psyllid instance

*Reply Payload*

- ``streams: [[stream_name (string)]]`` -- array of names of the streams

``node-list.[stream]``
----------------------
Returns a list of all the nodes in the indicated stream

*Reply Payload*

- ``nodes: [[node_name (string)]]`` -- array of names of the nodes

``filename.[file_number (optional)]``
------------
Returns the filename that will be written to by writters registered to ``file_number``.  Default for ``file_number`` is 0.

*Reply Payload*

- ``values: [[filename (string)]]`` -- Filename as the first element of the ``values`` array

``description.[file_number (optional)]``
---------------
Returns the description that will be written to the file header for file corresponding to ``file_number``.  Default for ``file_number`` is 0.

*Reply Payload*

- ``values: [[description (string)]]`` -- Description as the first element of the ``values`` array

``duration``
------------
Returns the run duration (in ms).

*Reply Payload*

- ``values: [[duration (unsigned int)]]`` -- Duration in ms as the first element of the ``values`` array

``use-monarch``
---------------
Returns the use-monarch flag.

*Reply Payload*

- ``values: [[flag (bool)]]`` -- Use-monarch flag as the first element of the ``values`` array


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

``active-config.[stream].[node]``
-------------------------------
Configures one or more parameters within an active DAQ node.  Takes effect immediately.  

*Payload*

- ``[node configuration (dictionary)]`` -- Parameters to set in the node

*Reply Payload*

- ``[the parameters that were set (dictionary)]`` -- Parameter name:value pairs that were set

``active-config.[stream].[node].[parameter]``
-------------------------------------------
Configure a single parameter in an active DAQ node.  Takes effect immediately.  
Please note that this action will not necessarily be useful for all node parameters (e.g. if a parameter is used once during initialization), and is not necessarily thread-safe.

*Payload*

- ``values: [[value]]`` -- Parameter value to be set as the first element of the ``values`` array.

``filename.[file_number (optional)]``
------------
Sets the filename (relative or absolute) that will be written to by the writers register to ``file_number``.  Default for ``file_number`` is 0.  Takes effect for the next run.

*Payload*

- ``values: [[filename (string)]]`` -- Filename

``description.[file_number (optional)]``
---------------
Sets the description that will be written to the file header for the file corresponding to ``file_number``.  Default for ``file_number`` is 0.  Takes effect for the next run.

*Payload*

- ``values: [[description (string)]]`` -- Description

*Reply Payload*

- ``[the parameter that was set as a dictionary]`` -- Parameter name:value pair that was set

``duration``
------------
Sets the run duration in ms. Takes effect for the next run.

*Payload*

- ``values: [[duration (unsigned int)]]`` -- Duration in ms

``use-monarch``
---------------
Sets the use-monarch flag. Takes effect for the next run.

*Payload*

- ``values: [[flag (bool)]]`` -- Flag value (true, false, 0, 1)


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

``run-daq-cmd.[stream].[node].[cmd]``
-------------------------------------
Instruct an active DAQ node to execute a particular command.  Please note that this action is not necessarily thread-safe.

*Payload*

- ``[command arguments (dictionary)]`` -- Any arguments needed for the execution of the command.

*Reply Payload*

- ``[the command configuration given to the node (dictionary)]`` -- Repeating what the node was told to do

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

<batch-command-key>
-------------------
All keys listed in the `batch-commands` node of the configuration are bound as command names and may be called.
These add one or more commands to the batch_executor queue; the return code indicates only that the actions were queued, nothing about their execution status.
The command `hard-abort` is defined in the default server_config to execute `stop-run`.
