/*
 * packet_eater_manager.cc
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#include "packet_eater_manager.hh"

#include "packet_eater.hh"
#include "psyllid_error.hh"

#include "logger.hh"


LOGGER( plog, "packet_eater_manager" );


namespace psyllid {

    packet_eater_manager::pe_ptr packet_eater_manager::get_packet_eater( const std::string& a_net_interface )
    {
        if( f_eaters.find( "a_net_interface" ) == f_eaters.end() )
        {
            // a new net interface is needed
            try
            {
                // create a new packet eater for this net interface
                pe_ptr t_new_eater = std::make_shared< packet_eater>( a_net_interface );

                // configure from the eater config
                scarab::param_node* t_config = f_eater_config.node_at( a_net_interface );
                if( t_config != nullptr )
                {
                    t_new_eater->set_timeout_sec( t_config->get_value( "timeout-sec", t_new_eater->get_timeout_sec() ) );
                    t_new_eater->set_n_blocks( t_config->get_value( "n-blocks", t_new_eater->get_n_blocks() ) );
                    t_new_eater->set_block_size( t_config->get_value( "block-size", t_new_eater->get_block_size() ) );
                    t_new_eater->set_frame_size( t_config->get_value( "frame-size", t_new_eater->get_frame_size() ) );
                }

                t_new_eater->initialize();

                return t_new_eater;
            }
            catch( error& e )
            {
                LERROR( plog, "Unable to create packet eater for net interface <" << a_net_interface << ">" );
                return pe_ptr();
            }
        }

        return f_eaters[ a_net_interface ];
    }

    void packet_eater_manager::remove_packet_eater( const std::string& a_net_interface )
    {
        std::map< std::string, pe_ptr >::iterator t_it = f_eaters.find( a_net_interface );
        if( t_it != f_eaters.end() )
        {
            f_eaters.erase( t_it );
        }
        return;
    }

    packet_eater_manager::packet_eater_manager() :
            f_eaters(),
            f_eater_config()
    {
        const scarab::param_node* t_config = scarab::global_config::get_instance()->retrieve().node_at( "packet-eaters" );
        if( t_config != nullptr )
        {
            f_eater_config = *t_config;
        }
    }

    packet_eater_manager::~packet_eater_manager()
    {
    }

} /* namespace psyllid */
