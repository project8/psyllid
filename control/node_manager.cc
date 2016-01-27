/*
 * node_manager.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "node_manager.hh"

#include "node_config_preset.hh"
#include "psyllid_error.hh"

#include "midge_error.hh"

#include "factory.hh"
#include "logger.hh"
#include "param.hh"

using std::string;

using scarab::param_array;
using scarab::param_node;
using scarab::param_value;

using midge::diptera;

namespace psyllid
{
    LOGGER( plog, "node_manager" );

    node_manager::node_manager() :
            f_midge( new diptera() ),
            f_must_reset_midge( false ),
            f_nodes(),
            f_connections()
    {
    }

    node_manager::~node_manager()
    {
    }

    void node_manager::use_preset( const std::string& a_name )
    {
        f_must_reset_midge = true;

        node_config_preset* t_preset = scarab::factory< node_config_preset >::get_instance()->create( a_name );

        t_preset->set_nodes( this );
        t_preset->set_connections( this );

        return;
    }

    void node_manager::reset_midge()
    {
        f_must_reset_midge = true;

        f_midge.reset( new diptera() );

        scarab::factory< midge::node >* t_node_factory = scarab::factory< midge::node >::get_instance();

        for( nodes_t::const_iterator t_node_it = f_nodes.begin(); t_node_it != f_nodes.end(); ++t_node_it )
        {
            string t_node_name = t_node_it->first;
            string t_node_type = t_node_it->second;

            midge::node* t_new_node = t_node_factory->create( t_node_type );
            if( t_new_node == NULL )
            {
                throw error() << "Unable to create node of type <" << t_node_type << ">";
            }
            t_new_node->set_name( t_node_name );

            try
            {
                f_midge->add( t_new_node );
                INFO( plog, "Added node <" << t_node_name << "> of type <" << t_node_type << ">" );
            }
            catch( midge::error& e )
            {
                delete t_new_node;
                throw error() << "Unable to add processor <" << t_node_name << ">: " << e.what();
            }
        }

        // Then deal with connections
        for( connections_t::const_iterator t_conn_it = f_connections.begin(); t_conn_it != f_connections.end(); ++t_conn_it )
        {
            try
            {
                f_midge->join( *t_conn_it );
            }
            catch( midge::error& e )
            {
                throw error() << "Unable to join nodes: " << e.what();
            }

            INFO( plog, "Node connection made:  <" << *t_conn_it << ">" );
        }

        f_must_reset_midge = false;
        return;
    }

    std::string node_manager::get_node_run_str() const
    {
        nodes_t::const_iterator t_node_it = f_nodes.begin();
        std::string t_run_str( t_node_it->first );
        for( ++t_node_it; t_node_it != f_nodes.end(); ++t_node_it )
        {
            t_run_str += diptera::separator() + t_node_it->first;
        }
        return t_run_str;
    }

} /* namespace psyllid */
