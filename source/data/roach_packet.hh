/*
 * roach_packet.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_ROACH_PACKET_HH_
#define PSYLLID_ROACH_PACKET_HH_

#include <cinttypes>
#include <cstddef> // for size_t

// number of samples in the roach_packet f_data array
#define PAYLOAD_SIZE 8192 // 1KB

// number of integers used in the pkt_in_batch counter
#define BATCH_COUNTER_SIZE 390626

namespace psyllid
{

    /*!
     @class roach_packet
     @author N.S. Oblath & A. Young

     @details
        The UDP packets have this structure:

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

        The fields are:
          - unix_time : The usual unix time meaning (seconds since 1 Jan 1970). At start-up the computer configures the "zero time" unix time, and from there the roach just keeps an internal counter.
          - pkt_in_batch : This is just a counter that increments with each pair of time/frequency packets sent out and wraps every 16 seconds (that is the shortest time that is both an integer number of seconds and integer number of packets). It counts to 390625 before wrapping to zero ( 390626 * (1 time + 1 freq packet) = 781252 packets in total every 16 seconds). A frequency and time packet that have the same counter value contains information on the same underlying data.
          - digital_id : This identifies the digital channel (one of three). In the bitcode and control software I call the channels 'a', 'b', and 'c', which will map to digital_id 0, 1, and 3, respectively.
          - if_id : Identifies the IF input on the roach, either 0 or 1, and corresponds to a label on the front panel. Currently only if0 used.
          - user_data_1 : This is a software configurable register within the roach reserved for whatever use you can think of. It is simply copied into the header of each packet.
          - user_data_0 : Ditto user_data_1
          - reserved_0 : This is a hard-coded 64-bit header inside the bitcode. Can also be put to use, or perhaps made software configurable.
          - reserved_1 : Ditto reserved_0, except it is only 63-bit.
          - freq_not_time : If 1, then the packet contains frequency-domain data, if 0 it contains time-domain data.

        32 bytes header + 8192 bytes payload = 8224 bytes in total. (Note network interfaces should be setup to handle MTU of this size - for high-speed network interfaces the default is usually much smaller. In any case, I've done this for the computer we acquired to go with the roach.)

        Note that you may need to do some byte-reordering depending on the machine you're working with. Each 64-bit word inside the packet is sent as big-endian, whereas the machine attached to the roach uses little-endian. So I had to do this (quick-and-dirty fix that I used for testing, there may be more elegant solutions):

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

        Now, interpreting the data:
        There are two kinds of packets: time-domain and frequency-domain.
          * Frequency domain:
             - There are 4096 x (8-bit real, 8-bit imaginary) spectrum samples within a packet.
             - Since the input signal is real-valued, the spectrum is symmetric and only the positive half of the spectrum is provided. So each packet contains the full spectral-information for each 8192-point FFT window. (There is a technical detail here, that is by the time the signal gets to the FFT it is already IQ-sampled and contains only information in the positive half-spectrum, but that doesn't affect anything.)
             - The samples are in canonical order, so the first sample is DC and the last sample is (100MHz - channel width), with every set of 8 sequential bytes written as a 64-bit big-endian word.
             - Sample rate at input to FFT is 200Msps.
          * Time-domain:
             - There are 4096 x (8-bit real, 8-bit imaginary) time-domain samples within a packet.
             - The time-domain data is IQ data of the positive half-spectrum of the signal going into the FFT, sampled at 100Msps. In other words, the 200Msps signal going into the FFT is tapped off, filtered and downconverted to shift information from 0MHz to +100MHz down to -50MHz to +50MHz, and then complex-sampled at 100Msps.
             - The samples are in correct order, so first sample first and last sample last, with every set of 8 sequential bytes written as a 64-bit big-endian word.
             - If you grab a time-domain and frequency-domain packet that have the same serial number (same unix_time and same pkt_in_batch), then you should essentially get the same spectrum (see figure attached). (This is not strictly true, the time-domain data passes through a second filter + downconversion, and then there's quantization in the roach FFT, but to first order this is true.)
     *
     */
    struct roach_packet
    {
        // first 64bit word
        uint32_t f_unix_time;
        uint32_t f_pkt_in_batch:20;
        uint32_t f_digital_id:6;
        uint32_t f_if_id:6;
        // second 64bit word
        uint32_t f_user_data_1;
        uint32_t f_user_data_0;
        // third 64bit word
        uint64_t f_reserved_0;
        // fourth 64bit word
        uint64_t f_reserved_1:63;
        uint8_t f_freq_not_time:1;
        // payload
        int8_t f_data[ PAYLOAD_SIZE ];
    };

    struct raw_roach_packet
    {
      uint64_t f_word_0;
      uint64_t f_word_1;
      uint64_t f_word_2;
      uint64_t f_word_3;
      char f_data[ PAYLOAD_SIZE ];
    };

    void byteswap_inplace( raw_roach_packet* a_pkt );


    class roach_packet_data
    {
        public:
            roach_packet_data();
            virtual ~roach_packet_data();

        public:
            uint32_t get_unix_time() const;
            void set_unix_time( uint32_t a_time );

            uint32_t get_pkt_in_batch() const;
            void set_pkt_in_batch( uint32_t a_pkt );

            uint32_t get_digital_id() const;
            void set_digital_id( uint32_t );

            uint32_t get_if_id() const;
            void set_if_id( uint32_t );

            uint32_t get_user_data_1() const;
            void set_user_data_1( uint32_t a_data );

            uint32_t get_user_data_0() const;
            void set_user_data_0( uint32_t a_data );

            uint64_t get_reserved_0() const;
            void set_reserved_0( uint64_t a_res );

            uint64_t get_reserved_1() const;
            void set_reserved_1( uint64_t a_res );

            bool get_freq_not_time() const;
            void set_freq_not_time( bool a_flag );

            const int8_t* get_raw_array() const;
            size_t get_raw_array_size() const;

        public:
            const roach_packet& packet() const;
            roach_packet& packet();

        protected:
            roach_packet f_packet;
    };


    inline uint32_t roach_packet_data::get_unix_time() const
    {
        return f_packet.f_unix_time;
    }

    inline void roach_packet_data::set_unix_time( uint32_t a_time )
    {
        f_packet.f_unix_time = a_time;
        return;
    }

    inline uint32_t roach_packet_data::get_pkt_in_batch() const
    {
        return f_packet.f_pkt_in_batch;
    }

    inline void roach_packet_data::set_pkt_in_batch( uint32_t a_pkt )
    {
        f_packet.f_pkt_in_batch = a_pkt;
        return;
    }

    inline uint32_t roach_packet_data::get_digital_id() const
    {
        return f_packet.f_digital_id;
    }

    inline void roach_packet_data::set_digital_id( uint32_t a_id )
    {
        f_packet.f_digital_id = a_id;
        return;
    }

    inline uint32_t roach_packet_data::get_if_id() const
    {
        return f_packet.f_if_id;
    }

    inline void roach_packet_data::set_if_id( uint32_t a_id )
    {
        f_packet.f_if_id = a_id;
        return;
    }

    inline uint32_t roach_packet_data::get_user_data_1() const
    {
        return f_packet.f_user_data_1;
    }

    inline void roach_packet_data::set_user_data_1( uint32_t a_data )
    {
        f_packet.f_user_data_1 = a_data;
        return;
    }

    inline uint32_t roach_packet_data::get_user_data_0() const
    {
        return f_packet.f_user_data_0;
    }

    inline void roach_packet_data::set_user_data_0( uint32_t a_data )
    {
        f_packet.f_user_data_0 = a_data;
        return;
    }

    inline uint64_t roach_packet_data::get_reserved_0() const
    {
        return f_packet.f_reserved_0;
    }

    inline void roach_packet_data::set_reserved_0( uint64_t a_res )
    {
        f_packet.f_reserved_0 = a_res;
        return;
    }

    inline uint64_t roach_packet_data::get_reserved_1() const
    {
        return f_packet.f_reserved_1;
    }

    inline void roach_packet_data::set_reserved_1( uint64_t a_res )
    {
        f_packet.f_reserved_1 = a_res;
        return;
    }

    inline bool roach_packet_data::get_freq_not_time() const
    {
        return f_packet.f_freq_not_time;
    }

    inline void roach_packet_data::set_freq_not_time( bool a_flag )
    {
        f_packet.f_freq_not_time = a_flag;
        return;
    }

    inline const int8_t* roach_packet_data::get_raw_array() const
    {
        return f_packet.f_data;
    }

    inline size_t roach_packet_data::get_raw_array_size() const
    {
        return PAYLOAD_SIZE;
    }

    inline const roach_packet& roach_packet_data::packet() const
    {
        return f_packet;
    }

    inline roach_packet& roach_packet_data::packet()
    {
        return f_packet;
    }

} /* namespace psyllid */

#endif /* PSYLLID_ROACH_PACKET_HH_ */
