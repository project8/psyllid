/*
 * udp_receiver.cc
 *
 *  Created on: Dec 25, 2015
 *      Author: nsoblath
 */

#include "udp_receiver.hh"

namespace psyllid
{

    udp_receiver::udp_receiver() :
            f_length( 10 )
    {
    }

    udp_receiver::~udp_receiver()
    {
    }

    void udp_receiver::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
        return;
    }

    void udp_receiver::execute()
    {
        index_t t_index;

        roach_packet* t_current_data;

        t_current_data = out_stream< 0 >().data();

        // do any preparation of the data

        out_stream< 0 >().set( stream::s_start );

        while( true )
        {
            if( (out_stream< 0 >().get() == stream::s_stop) || (false /* some other condition for stopping */) )
            {
                out_stream< 0 >().set( stream::s_stop );
                out_stream< 0 >().set( stream::s_exit );
                return;
            }

            t_current_data = out_stream< 0 >().data();

            // load the data object

            out_stream< 0 >().set( stream::s_run );
        }

        return;
    }

    void udp_receiver::finalize()
    {
        out_buffer< 0 >().finalize();
        return;
    }

} /* namespace psyllid */
