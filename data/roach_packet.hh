/*
 * roach_packet.hh
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#ifndef DATA_ROACH_PACKET_HH_
#define DATA_ROACH_PACKET_HH_

#include <cinttypes>

#define PAYLOAD_SIZE 8192 // 1KB

namespace psyllid
{

    struct roach_packet {
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
        uint64_t f_freq_not_time:1;
        // payload
        char f_data[PAYLOAD_SIZE];
    };

} /* namespace psyllid */

#endif /* DATA_ROACH_PACKET_HH_ */
