=============
Roach Packets
=============

The ROACH2 for Project 8 sends UDP packets over a 10 GBe connection.

As configured for Phase 2 of Project 8, time-domain and frequency-domain packets alternate for each channel.

These notes are based on an email from André to Noah (1/21/16).

Structure
=========

Definition
----------
::

  #define PAYLOAD_SIZE 8192 // 1KB
  #define PACKET_SIZE sizeof(packet_t)
  typedef struct packet {
	// first 64bit word
	uint32_t unix_time;
	uint32_t pkt_in_batch:20;
	uint32_t digital_id:6;
	uint32_t if_id:6;
	// second 64bit word
	uint32_t user_data_1;
	uint32_t user_data_0;
	// third 64bit word
	uint64_t reserved_0;
	// fourth 64bit word
	uint64_t reserved_1:63;
	uint64_t freq_not_time:1;
	// payload
	char data[PAYLOAD_SIZE];
  } packet_t;


Fields
------

unix_time
  The usual unix time meaning (seconds since 1 Jan 1970). At start-up the computer configures the "zero time" unix time, and from there the roach just keeps an internal counter.
pkt_in_batch
  This is just a counter that increments with each pair of time/frequency packets sent out and wraps every 16 seconds (that is the shortest time that is both an integer number of seconds and integer number of packets). It counts to 390625 before wrapping to zero ( 390626 * (1 time + 1 freq packet) = 781252 packets in total every 16 seconds). A frequency and time packet that have the same counter value contains information on the same underlying data.
digital_id
  This identifies the digital channel (one of three). In the bitcode and control software I call the channels 'a', 'b', and 'c', which will map to digital_id 0, 1, and 3, respectively.
if_id
  Identifies the IF input on the roach, either 0 or 1, and corresponds to a label on the front panel. Currently only if0 used.
user_data_1
  This is a software configurable register within the roach reserved for whatever use you can think of. It is simply copied into the header of each packet.
user_data_0
  Ditto user_data_1
reserved_0
  This is a hard-coded 64-bit header inside the bitcode. Can also be put to use, or perhaps made software configurable.
reserved_1
  Ditto reserved_0, except it is only 63-bit.
freq_not_time
  If 1, then the packet contains frequency-domain data, if 0 it contains time-domain data.


Size
----

32 bytes header + 8192 bytes payload = 8224 bytes in total.

*Note*: network interfaces should be setup to handle MTU of this size - for high-speed network interfaces the default is usually much smaller. In any case, I've done this for the computer we acquired to go with the roach.)


Byte Reordering
---------------

You may need to do some byte-reordering depending on the machine you're working with. Each 64-bit word inside the packet is sent as big-endian, whereas the machine attached to the roach uses little-endian. So I had to do this (quick-and-dirty fix that I used for testing, there may be more elegant solutions)::

  typedef struct raw_packet {
    uint64_t _h0;
    uint64_t _h1;
    uint64_t _h2;
    uint64_t _h3;
    ...
  } raw_packet_t;

  packet_t *ntoh_packet(raw_packet_t *pkt) {
    pkt->_h0 = be64toh(pkt->_h0);
    pkt->_h1 = be64toh(pkt->_h1);
    pkt->_h2 = be64toh(pkt->_h2);
    pkt->_h3 = be64toh(pkt->_h3);
    ...
    return (packet_t *)pkt;
  }


Interpreting the Data
=====================

There are two kinds of packets: time-domain and frequency-domain.


Frequency Domain
----------------

There are 4096 x (8-bit real, 8-bit imaginary) spectrum samples within a packet.

Since the input signal is real-valued, the spectrum is symmetric and only the positive half of the spectrum is provided. So each packet contains the full spectral-information for each 8192-point FFT window. (There is a technical detail here, that is by the time the signal gets to the FFT it is already IQ-sampled and contains only information in the positive half-spectrum, but that doesn't affect anything.)

The samples are in canonical order, so the first sample is DC and the last sample is ``(100 MHz - channel width)``.

Sample rate at input to FFT is 200 Msps.

Time Domain
-----------

There are 4096 x (8-bit real, 8-bit imaginary) time-domain samples within a packet.

The time-domain data is IQ data of the positive half-spectrum of the signal going into the FFT, sampled at 100 Msps. In other words, the 200 Msps signal going into the FFT is tapped off, filtered and downconverted to shift information from 0 MHz to +100 MHz down to -50 MHz to +50 MHz, and then complex-sampled at 100 Msps.
The samples are in correct order, so first sample first and last sample last.
If you grab a time-domain and frequency-domain packet that have the same serial number (same ``unix_time`` and same ``pkt_in_batch``), then you should essentially get the same spectrum. (This is not strictly true, the time-domain data passes through a second filter + downconversion, and then there's quantization in the roach FFT, but to first order this is true.)