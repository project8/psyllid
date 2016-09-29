/*
 * fast_packet_acq.cc
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#include "fast_packet_acq.hh"

#include "packet_distributor.hh"
#include "packet_eater.hh"
#include "psyllid_error.hh"

#include "logger.hh"

#include <thread>

LOGGER( plog, "fast_packet_acq" );


namespace psyllid
{

    fast_packet_acq::fast_packet_acq( const std::string& a_interface ) :
            f_is_initialized( false ),
            f_is_running( false ),
            f_eater( nullptr ),
            f_distributor( nullptr )
    {
        set_name( a_interface );
    }

    fast_packet_acq::~fast_packet_acq()
    {
        LWARN( plog, "Fast Packet Acq Destructor!" );
    }

    void fast_packet_acq::initialize()
    {
        if( f_is_initialized ) return;
        try
        {
            // create a new packet eater for this net interface
            f_eater = std::make_shared< packet_eater >( f_name );

            // create a new packet distributor for the interface and connect the distributor to the eater
            f_distributor = std::make_shared< packet_distributor >();

            f_distributor->set_ip_buffer( &f_eater->buffer() );

            // initialize the eater
            if( ! f_eater->initialize() )
            {
                throw error() << "[fast_packet_acq] Unable to initialize the eater";
            }
            f_distributor->initialize();

            f_is_initialized = true;
        }
        catch( error& e )
        {
            LERROR( plog, "Exception thrown while attempting to create and initialize eater and distributor: " << e.what() );
            throw( e );
        }

        return;
    }

    bool fast_packet_acq::activate_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size )
    {
        if( ! f_is_initialized )
        {
            LERROR( plog, "FPA <" << f_name << "> must be initialized before opening a port (" << a_port << ")" );
        }
        if( ! f_distributor->open_port( a_port, a_iterator, a_buffer_size ) )
        {
            LERROR( plog, "Unable to open port <" << a_port << "> on interface <" << f_name << ">" );
            return false;
        }
        LDEBUG( plog, "Port <" << a_port << "> has been opened on interface <" << f_name << ">" );

        return true;
    }

    void fast_packet_acq::execute()
    {
        if( f_is_running ) return;
        f_is_running = true;

        // start distributor before eater so that the read iterator is positioned correctly behind the write iterator before the eater starts
        LINFO( plog, "Starting distributor for interface <" << f_name << ">" );
        std::thread t_dist_thread( &packet_distributor::execute, f_distributor );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        LINFO( plog, "Starting eater for interface <" << f_name << ">" );
        std::thread t_eater_thread( &packet_eater::execute, f_eater );
        LDEBUG( plog, "FPA threads started" );

        // delay to allow the threads to spin up
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

        LDEBUG( plog, "Waiting for FPA threads to finish" );
        t_eater_thread.join();
        LINFO( plog, "Eater for interface <" << f_name << "> has finished" );
        t_dist_thread.join();
        LINFO( plog, "Distributor for interface <" << f_name << "> has finished" );

        f_is_running = false;
        return;
    }

    void fast_packet_acq::finalize()
    {
        return;
    }

    void fast_packet_acq::do_cancellation()
    {
        f_eater->cancel();
        f_distributor->cancel();
        return;
    }

    void fast_packet_acq::do_reset_cancellation()
    {
        f_eater->reset_cancel();
        f_distributor->reset_cancel();
        return;
    }



    fast_packet_acq_builder::fast_packet_acq_builder() :
            _node_builder< fast_packet_acq >()
    {
    }

    fast_packet_acq_builder::~fast_packet_acq_builder()
    {
    }

    void fast_packet_acq_builder::apply_config( fast_packet_acq* a_node, const scarab::param_node& a_config )
    {
        LDEBUG( plog, "Configuring fast_packet_acq with :\n" << a_config );
        a_node->set_name( a_config.get_value( "interface", a_node->net_interface() ) );
        a_node->eater()->set_timeout_sec( a_config.get_value( "timeout-sec", a_node->eater()->get_timeout_sec() ) );
        a_node->eater()->set_n_blocks( a_config.get_value( "n-blocks", a_node->eater()->get_n_blocks() ) );
        a_node->eater()->set_block_size( a_config.get_value( "block-size", a_node->eater()->get_block_size() ) );
        a_node->eater()->set_frame_size( a_config.get_value( "frame-size", a_node->eater()->get_frame_size() ) );
        return;
    }


} /* namespace psyllid */
