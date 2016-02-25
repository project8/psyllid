/*
 * roach_packet.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_ROACH_PACKET_HH_
#define PSYLLID_ROACH_PACKET_HH_

#include <cinttypes>

// number of samples in the roach_packet f_data array
#define PAYLOAD_SIZE 8192 // 1KB

namespace psyllid
{

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
