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


LOGGER( plog, "fast_packet_acq" );


namespace psyllid
{

    fast_packet_acq::fast_packet_acq() :
            f_net_interface( "eth1" ),
            f_eater( nullptr ),
            f_distributor( nullptr )
    {
    }

    fast_packet_acq::~fast_packet_acq()
    {
    }

    void fast_packet_acq::initialize()
    {
        try
        {
            // create a new packet eater for this net interface
            f_eater = std::make_shared< packet_eater >( f_net_interface );

            // create a new packet distributor for the interface and connect the distributor to the eater
            f_distributor = std::make_shared< packet_distributor >( &f_eater->buffer() );
        }
        catch( error& e )
        {
            LERROR( plog, "Exception thrown while attempting to create eater and distributor: " << e.what() );
            throw( e );
        }

        // initialize the eater
        if( ! f_eater->initialize() )
        {
            throw error() << "[fast_packet_acq] Unable to initialize the eater";
        }
        return;
    }

    bool fast_packet_acq::activate_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size )
    {
        if( ! f_distributor->open_port( a_port, a_iterator, a_buffer_size ) )
        {
            LERROR( plog, "Unable to open port <" << a_port << "> on interface <" << f_net_interface << ">" );
            return false;
        }

        return true;
    }

    void fast_packet_acq::execute()
    {
        // start distributor before eater so that the read iterator is positioned correctly behind the write iterator before the eater starts
        f_distributor->execute();
        f_eater->execute();
        return;
    }

    void fast_packet_acq::finalize()
    {
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
        a_node->net_interface() = a_config.get_value( "net-interface", a_node->net_interface() );
        a_node->eater()->set_timeout_sec( a_config.get_value( "timeout-sec", a_node->eater()->get_timeout_sec() ) );
        a_node->eater()->set_n_blocks( a_config.get_value( "n-blocks", a_node->eater()->get_n_blocks() ) );
        a_node->eater()->set_block_size( a_config.get_value( "block-size", a_node->eater()->get_block_size() ) );
        a_node->eater()->set_frame_size( a_config.get_value( "frame-size", a_node->eater()->get_frame_size() ) );
        return;
    }


} /* namespace psyllid */
