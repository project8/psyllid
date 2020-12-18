# Psyllid Examples

## Configuration Files

This directory contains example configuration files for a number of different basic setups:

* `eb_fmt_1ch_fpa.yaml`: Triggered events, 1 channel, fast packet-acquisition (linux only)
* `eb_fmt_1ch_socket.yaml`: Triggered events, 1 channel, standard networing
* `fmt_1ch_fpa.yaml`: Triggered, 1 channel, fast packet-acquisition (linux only)
* `fmt_1ch_socket.yaml`: Triggered, 1 channel, standard networking
* `str_1ch_fpa.yaml`: Streaming, 1 channel, fast packet-acquisition (linux only)
* `str_1ch_dataprod.yaml`: Streaming, 1 channel, using the data producer
* `str_1ch_socket_batch.yaml`: Streaming, 1 channel, standard networking, using batch commands
* `str_1ch_socket_custom.yaml`: Streaming, 1 channel, standard networking, preset customization example
* `str_1ch_socket.yaml`: Streaming, 1 channel, standard networking
* `str_3ch_fpa.yaml`: Streaming, 3 channels, fast packet-acquisition (linux only)

## Executables

Specialized executables can be created instead of using the general Psyllid application, if so desired.
Note that this is not the recommended method of running, as you can get the same performance from using `psyllid`.

* `roach_daq_1chan.cc`: 1-channel triggered application: simple midge implementation.
* `roach_daq_streaming_1chan.cc`: 1-channel time-series streaming; application is based on the Scarab application framework.
