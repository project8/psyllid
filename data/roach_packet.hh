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
        char f_data[ PAYLOAD_SIZE ];
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
            uint32_t get_pkt_in_batch() const;
            uint32_t get_digital_id() const;
            uint32_t get_if_id() const;
            uint32_t get_user_data_1() const;
            uint32_t get_user_data_0() const;
            uint64_t get_reserved_0() const;
            uint64_t get_reserved_1() const;
            bool get_freq_not_time() const;

            const char* get_raw_array() const;
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

    inline uint32_t roach_packet_data::get_pkt_in_batch() const
    {
        return f_packet.f_pkt_in_batch;
    }

    inline uint32_t roach_packet_data::get_digital_id() const
    {
        return f_packet.f_digital_id;
    }

    inline uint32_t roach_packet_data::get_if_id() const
    {
        return f_packet.f_if_id;
    }

    inline uint32_t roach_packet_data::get_user_data_1() const
    {
        return f_packet.f_user_data_1;
    }

    inline uint32_t roach_packet_data::get_user_data_0() const
    {
        return f_packet.f_user_data_0;
    }

    inline uint64_t roach_packet_data::get_reserved_0() const
    {
        return f_packet.f_reserved_0;
    }

    inline uint64_t roach_packet_data::get_reserved_1() const
    {
        return f_packet.f_reserved_1;
    }

    inline bool roach_packet_data::get_freq_not_time() const
    {
        return f_packet.f_freq_not_time;
    }

    inline const char* roach_packet_data::get_raw_array() const
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
