/*
 * node_config_preset.cc
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#include "node_config_preset.hh"

#include "node_manager.hh"

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

    void node_config_preset::set_nodes( node_manager* a_manager ) const
    {
        for( nodes_t::const_iterator t_node_it = f_nodes.begin(); t_node_it != f_nodes.end(); ++t_node_it )
        {
            a_manager->add_node( t_node_it->second, t_node_it->first );
        }
        return;
    }

    void node_config_preset::set_connections( node_manager* a_manager ) const
    {
        for( connections_t::const_iterator t_conn_it = f_connections.begin(); t_conn_it != f_connections.end(); ++t_conn_it )
        {
            a_manager->add_connection( *t_conn_it );
        }
        return;
    }


} /* namespace psyllid */
