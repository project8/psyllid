/*
 * roach_time_data.cc
 *
 *  Created on: Mar 21, 2025
 *      Author: pkolbeck
 */

 #include "roach_packet.hh"
 #include "iq_time_data.hh"
 #include "roach_time_data.hh"

 #include "byte_swap.hh"
 
 namespace psyllid
 {
 
     roach_time_data::roach_time_data() :
             roach_packet_data(), 
             iq_time_data( false ) 
     {
        set_array_ptr( reinterpret_cast<iq_t*>( f_packet.f_data ) );
     }
 
     roach_time_data::~roach_time_data()
     {}
 
 
 }
 
 
 