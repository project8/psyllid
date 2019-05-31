/*
 * data_producer.cc
 *
 *  Created on: May 31, 2019
 *      Author: obla999
 */

#include "data_producer.hh"

#include "logger.hh"


LOGGER( plog, "data_producer" );

using midge::stream;

namespace psyllid
{

    data_producer::data_producer() :
            f_length( 10 ),
            f_max_packet_size( 1048576 )
    {
    }

    data_producer::~data_producer()
    {
    }

    void data_producer::initialize()
    {
        out_buffer< 0 >().initialize( f_length );
    }

    void data_producer::execute( midge::diptera* a_midge )
    {
        LDEBUG( plog, "Executing the packet_receiver_socket" );

        try
        {
            memory_block* t_block = nullptr;

            //LDEBUG( plog, "Server is listening" );

            if( ! out_stream< 0 >().set( stream::s_start ) ) return;

            ssize_t t_size_received = 0;

            LINFO( plog, "Starting main loop; waiting for packets" );
            while( ! is_canceled() )
            {
                t_block = out_stream< 0 >().data();
                t_block->resize( f_max_packet_size );

                t_size_received = 0;

                if( out_stream< 0 >().get() == stream::s_stop )
                {
                    LWARN( plog, "Output stream(s) have stop condition" );
                    break;
                }

                if( ! out_stream< 0 >().set( stream::s_run ) )
                {
                    LERROR( plog, "Exiting due to stream error" );
                    break;
                }
            }

            LINFO( plog, "Data producer is exiting" );

            // normal exit condition
            LDEBUG( plog, "Stopping output stream" );
            if( ! out_stream< 0 >().set( stream::s_stop ) ) return;

            LDEBUG( plog, "Exiting output stream" );
            out_stream< 0 >().set( stream::s_exit );

            return;
        }
        catch(...)
        {
            a_midge->throw_ex( std::current_exception() );
        }
    }

    void data_producer::finalize()
    {
        out_buffer< 0 >().finalize();

        return;
    }


} /* namespace dripline */
