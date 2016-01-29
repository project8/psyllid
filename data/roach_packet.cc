/*
 * roach_packet.cc
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#include "roach_packet.hh"

#include "byte_swap.hh"

namespace psyllid
{
    roach_packet_data::roach_packet_data() :
            f_packet()
    {}

    roach_packet_data::~roach_packet_data()
    {}

    void byteswap_inplace( raw_roach_packet* a_pkt )
    {
        a_pkt->f_word_0 = bswap_64( a_pkt->f_word_0 );
        a_pkt->f_word_1 = bswap_64( a_pkt->f_word_1 );
        a_pkt->f_word_2 = bswap_64( a_pkt->f_word_2 );
        a_pkt->f_word_3 = bswap_64( a_pkt->f_word_3 );
        return;
    }

}


