/*
 * stream_preset.cc
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#include "stream_preset.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "param.hh"

namespace psyllid
{
    LOGGER( plog, "stream_preset" );

    //********************
    // stream_preset
    //********************

    stream_preset::stream_preset() :
            f_type( "unknown" ),
            f_nodes(),
            f_connections()
    {
    }

    stream_preset::stream_preset( const std::string& a_type ) :
            f_type( a_type ),
            f_nodes(),
            f_connections()
    {
    }

    stream_preset::stream_preset( const stream_preset& a_orig ) :
            f_type( a_orig.f_type ),
            f_nodes( a_orig.f_nodes ),
            f_connections( a_orig.f_connections )
    {
    }

    stream_preset::~stream_preset()
    {
    }

    stream_preset& stream_preset::operator=( const stream_preset& a_rhs )
    {
        f_type = a_rhs.f_type;
        f_nodes = a_rhs.f_nodes;
        f_connections = a_rhs.f_connections;
        return *this;
    }

    void stream_preset::node( const std::string& a_type, const std::string& a_name )
    {
        if( f_nodes.find( a_name ) != f_nodes.end() )
        {
            throw error() << "Invalid preset: node is already present: <" + a_name + "> of type <" + a_type + ">";
        }
        f_nodes.insert( nodes_t::value_type( a_name, a_type ) );
        return;
    }

    void stream_preset::connection( const std::string& a_conn )
    {
        if( f_connections.find( a_conn ) != f_connections.end() )
        {
            throw error() << "Invalid preset; connection is already present: <" + a_conn + ">";
        }
        f_connections.insert( a_conn );
    }


    //****************************
    // node_config_runtime_preset
    //****************************

    std::map< std::string, node_config_runtime_preset> node_config_runtime_preset::s_runtime_presets;

    node_config_runtime_preset::node_config_runtime_preset() :
            stream_preset()
    {
    }

    node_config_runtime_preset::node_config_runtime_preset( const std::string& a_type ) :
            stream_preset( a_type )
    {
        f_nodes = s_runtime_presets.at( a_type ).f_nodes;
        f_connections = s_runtime_presets.at( a_type ).f_connections;
    }

    node_config_runtime_preset::node_config_runtime_preset( const node_config_runtime_preset& a_orig ) :
            stream_preset( a_orig )
    {
    }

    node_config_runtime_preset::~node_config_runtime_preset()
    {
    }

    node_config_runtime_preset& node_config_runtime_preset::operator=( const node_config_runtime_preset& a_rhs )
    {
        stream_preset::operator=( a_rhs );
        return *this;
    }

    bool node_config_runtime_preset::add_preset( const scarab::param_node* a_preset_node )
    {
        if( a_preset_node == nullptr )
        {
            LERROR( plog, "Null preset config received" );
            return false;
        }

        if( ! a_preset_node->has( "type" ) )
        {
            LERROR( plog, "Preset must have a type.  Preset config:\n" << *a_preset_node );
            return false;
        }
        std::string t_preset_type = a_preset_node->get_value( "type" );

        const scarab::param_array* t_nodes_array = a_preset_node->array_at( "nodes" );
        if( t_nodes_array == nullptr )
        {
            LERROR( plog, "No \"nodes\" configuration was present for preset <" << t_preset_type << ">" );
            return false;
        }

        node_config_runtime_preset t_new_preset( t_preset_type );

        std::string t_type;
        for( scarab::param_array::const_iterator t_nodes_it = t_nodes_array->begin(); t_nodes_it != t_nodes_array->end(); ++t_nodes_it )
        {
            if( ! (*t_nodes_it)->is_node() )
            {
                LERROR( plog, "Invalid node specification in preset <" << t_preset_type << ">" );
                return false;
            }

            t_type = (*t_nodes_it)->as_node().get_value( "type", "" );
            if( t_type.empty() )
            {
                LERROR( plog, "No type given for one of the nodes in preset <" << t_preset_type << ">" );
                return false;
            }

            LDEBUG( plog, "Adding node <" << t_type << ":" << (*t_nodes_it)->as_node().get_value( "name", t_type ) << "> to preset <" << t_preset_type << ">" );
            t_new_preset.node( t_type, (*t_nodes_it)->as_node().get_value( "name", t_type ) );
        }

        const scarab::param_array* t_conn_array = a_preset_node->array_at( "connections" );
        if( t_conn_array == nullptr )
        {
            LDEBUG( plog, "Preset <" << t_preset_type << "> is being setup with no connections" );
        }
        else
        {
            for( scarab::param_array::const_iterator t_conn_it = t_conn_array->begin(); t_conn_it != t_conn_array->end(); ++t_conn_it )
            {
                if( ! (*t_conn_it)->is_value() )
                {
                    LERROR( plog, "Invalid connection specification in preset <" << t_preset_type << ">" );
                    return false;
                }

                LDEBUG( plog, "Adding connection <" << (*t_conn_it)->as_value().as_string() << "> to preset <" << t_preset_type << ">");
                t_new_preset.connection( (*t_conn_it)->as_value().as_string() );
            }
        }

        s_runtime_presets[ t_preset_type ] = t_new_preset;
        LINFO( plog, "Preset <" << t_preset_type << "> is now available" );

        return true;
    }

} /* namespace psyllid */
