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

    REGISTER_NODE_AND_BUILDER( data_producer, "data-producer", data_producer_binding );

    data_producer::data_producer() :
            f_length( 10 ),
            f_data_size( 8224 ),
            f_master_packet()
    {
        f_master_packet.set_freq_not_time( false );
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
        LDEBUG( plog, "Executing the data_producer" );

        try
        {
            memory_block* t_block = nullptr;

            //LDEBUG( plog, "Server is listening" );

            if( ! out_stream< 0 >().set( stream::s_start ) ) return;

            ssize_t t_size_received = 0;

            LINFO( plog, "Starting main loop; sending packets" );
            //unsigned count = 0;
            while( ! is_canceled() )
            {
                t_block = out_stream< 0 >().data();
                if( t_block->get_n_bytes() != f_data_size )
                {
                    initialize_block( t_block );
                }

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

    void data_producer::initialize_block( memory_block* a_block )
    {
        a_block->resize( f_data_size );
        a_block->set_n_bytes_used( f_data_size );

        roach_packet* t_roach_packet = reinterpret_cast< roach_packet* >( a_block->block() );
        ::memcpy( t_roach_packet, &f_master_packet.packet(), f_data_size );

        return;
    }

    data_producer_binding::data_producer_binding() :
            sandfly::_node_binding< data_producer, data_producer_binding >()
    {
    }

    data_producer_binding::~data_producer_binding()
    {
    }

    void data_producer_binding::do_apply_config( data_producer* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring data_producer with:\n" << a_config );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        a_node->set_data_size( a_config.get_value( "data-size", a_node->get_data_size() ) );
        return;
    }

    void data_producer_binding::do_dump_config( const data_producer* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping data_producer configuration" );
        a_config.add( "length", scarab::param_value( a_node->get_length() ) );
        a_config.add( "data-size", scarab::param_value( a_node->get_data_size() ) );
        return;

    }

} /* namespace dripline */
