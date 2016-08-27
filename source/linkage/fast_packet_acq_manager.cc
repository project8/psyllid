/*
 * fast_packet_acq_manager.cc
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#include "fast_packet_acq_manager.hh"

#include "packet_distributor.hh"
#include "packet_eater.hh"
#include "psyllid_error.hh"

#include "logger.hh"


LOGGER( plog, "fast_packet_acq_manager" );


namespace psyllid
{

    bool fast_packet_acq_manager::activate_port( const std::string& a_net_interface, unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size )
    {
        route_map::iterator t_route_it = f_routes.find( "a_net_interface" );
        if( t_route_it == f_routes.end() )
        {
            // a new net interface is needed
            try
            {
                fpa_route t_new_route;

                // create a new packet eater for this net interface
                t_new_route.f_eater = std::make_shared< packet_eater >( a_net_interface );

                // configure from the eater config
                scarab::param_node* t_config = f_config.node_at( a_net_interface );
                if( t_config != nullptr )
                {
                    t_new_route.f_eater->set_timeout_sec( t_config->get_value( "timeout-sec", t_new_route.f_eater->get_timeout_sec() ) );
                    t_new_route.f_eater->set_n_blocks( t_config->get_value( "n-blocks", t_new_route.f_eater->get_n_blocks() ) );
                    t_new_route.f_eater->set_block_size( t_config->get_value( "block-size", t_new_route.f_eater->get_block_size() ) );
                    t_new_route.f_eater->set_frame_size( t_config->get_value( "frame-size", t_new_route.f_eater->get_frame_size() ) );
                }

                // initialize the eater
                if( ! t_new_route.f_eater->initialize() )
                {
                    LERROR( plog, "Unable to initialize eater for interface <" << a_net_interface << ">" );
                    return false;
                }

                // create a new packet distributor for the interface and connect the distributor to the eater
                t_new_route.f_distributor = std::make_shared< packet_distributor >( &t_new_route.f_eater->buffer() );

                f_routes[ a_net_interface ] = t_new_route;
                t_route_it = f_routes.find( a_net_interface );
            }
            catch( error& e )
            {
                LERROR( plog, "Caught exception while attempting to setup route for net interface <" << a_net_interface << ">: " << e.what() );
                return false;
            }
        }

        // route for this interface exists
        // now activate the port

        if( ! t_route_it->second.f_distributor->open_port( a_port, a_iterator, a_buffer_size ) )
        {
            LERROR( plog, "Unable to open port <" << a_port << "> on interface <" << a_net_interface << ">" );
            return false;
        }

        return true;
    }

    void fast_packet_acq_manager::remove_interface( const std::string& a_net_interface )
    {
        route_map::iterator t_it = f_routes.find( a_net_interface );
        if( t_it != f_routes.end() )
        {
            f_routes.erase( t_it );
        }
        return;
    }

    fast_packet_acq_manager::fast_packet_acq_manager() :
            f_routes(),
            f_config()
    {
        const scarab::param_node* t_config = scarab::global_config::get_instance()->retrieve().node_at( "fast-packet-acq" );
        if( t_config != nullptr )
        {
            f_config = *t_config;
        }
    }

    fast_packet_acq_manager::~fast_packet_acq_manager()
    {
    }

} /* namespace psyllid */
