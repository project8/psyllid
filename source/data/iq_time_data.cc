/*
 * iq_time_data.cc
 *
 *  Created on: Mar 21, 2025
 *      Author: pkolbeck
 */

#include "iq_time_data.hh"

namespace psyllid
{

    iq_time_data::iq_time_data( bool has_data ) :
            f_has_data( has_data ),
            f_array( ),
            f_array_ptr( ), 
            f_array_size( PAYLOAD_SIZE / 2 ),
            f_pkt_in_session( 0 )
    {
        if( has_data )
        {
            iq_time_data::set_array_ptr( f_array );
        }
    }

    iq_time_data::~iq_time_data()
    {
    }

} /* namespace psyllid */
