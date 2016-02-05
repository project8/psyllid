/*
 * node_config_preset.cc
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#include "node_config_preset.hh"

#include "node_manager.hh"
#include "psyllid_error.hh"

namespace psyllid
{

    node_config_preset::node_config_preset() :
            f_nodes(),
            f_connections()
    {
    }

    node_config_preset::~node_config_preset()
    {
    }

    void node_config_preset::node( const std::string& a_type, const std::string& a_name )
    {
        if( f_nodes.find( a_name ) != f_nodes.end() )
        {
            throw error() << "Invalid preset: node is already present: <" + a_name + "> of type <" + a_type + ">";
        }
        f_nodes.insert( nodes_t::value_type( a_type, a_name ) );
        return;
    }

    void node_config_preset::connection( const std::string& a_conn )
    {
        if( f_connections.find( a_conn ) != f_connections.end() )
        {
            throw error() << "Invalid preset; connection is already present: <" + a_conn + ">";
        }
        f_connections.insert( a_conn );
    }



} /* namespace psyllid */
