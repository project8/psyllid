/*
 * stream_manager.cc
 *
 *  Created on: Jan 5, 2017
 *      Author: obla999
 */

#include "stream_manager.hh"

#include "node_builder.hh"
#include "node_config_preset.hh"
#include "psyllid_error.hh"

#include "node.hh"

#include "logger.hh"

#include <boost/algorithm/string/replace.hpp>

namespace psyllid
{
    LOGGER( plog, "stream_manager" );

    stream_manager::stream_manager() :
            f_streams(),
            f_manager_mutex(),
            f_midge(),
            f_must_reset_midge( true ),
            f_midge_mutex()
    {
    }

    stream_manager::~stream_manager()
    {
        for( streams_t::iterator t_stream_it = f_streams.begin(); t_stream_it != f_streams.end(); ++t_stream_it )
        {
            for( stream_template::nodes_t::iterator t_node_it = t_stream_it->f_nodes.begin(); t_node_it != t_stream_it->f_nodes.end(); ++t_node_it )
            {
                delete t_node_it->second;
                t_node_it->second = nullptr;
            }
        }
    }

    int stream_manager::add_stream( const scarab::param_node* a_node )
    {
        // do not need to lock the mutex here because we're not doing anything to the stream_manager until inside _add_stream()

        if( a_node == nullptr )
        {
            LWARN( plog, "Null config received" );
            return -1;
        }

        const scarab::param* t_preset_param = a_node->at( "preset" );

        if( t_preset_param == nullptr )
        {
            LWARN( plog, "No preset specified" );
            return -1;
        }
        else if( t_preset_param->is_node() )
        {
            const scarab::param_node* t_preset_param_node = &t_preset_param->as_node();
            if( ! node_config_runtime_preset::add_preset( t_preset_param_node ) )
            {
                LWARN( plog, "Runtime preset could not be added" );
                return -1;
            }
            // "name" is guaranteed to be there by the successful completion of node_config_runtime_preset::add_preset
            return _add_stream( t_preset_param_node->get_value( "name" ), a_node );
        }
        else if( t_preset_param->is_value() )
        {
            return _add_stream( t_preset_param->as_value().as_string(), a_node );
        }
        else
        {
            LWARN( plog, "Invalid preset specification" );
            return -1;
        }
    }

    void stream_manager::remove_stream( unsigned a_stream_no )
    {
        if( a_stream_no >= f_streams.size() )
        {
            LWARN( plog, "Stream <" << a_stream_no << "> does not exist" );
            return;
        }

        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;

        // delete node_builder objects
        streams_t::iterator t_to_erase = f_streams.begin() + a_stream_no;
        for( stream_template::nodes_t::iterator t_node_it = t_to_erase->f_nodes.begin(); t_node_it != t_to_erase->f_nodes.end(); ++t_node_it )
        {
            delete t_node_it->second;
            t_node_it->second = nullptr;
        }

        f_streams.erase( t_to_erase );

        return;
    }

    int stream_manager::_add_stream( const std::string& a_name, const scarab::param_node* a_node )
    {
        // do not need to lock the mutex here because we're not doing anything to the stream_manager until later

        node_config_preset* t_preset = scarab::factory< node_config_preset, const std::string& >::get_instance()->create( a_name, a_name );

        if( t_preset == nullptr )
        {
            LWARN( plog, "Unable to create preset called <" << a_name << ">. The name may not be registered or there may be a typo.");
            return -1;
        }

        // lock the mutex here so that we know which stream number this will be if it succeeds
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        unsigned t_stream_no = f_streams.size();
        std::map< std::string, std::string > t_name_replacements;

        LINFO( plog, "Preparing stream <" << t_stream_no << ">");

        stream_template t_stream;

        typedef node_config_preset::nodes_t preset_nodes_t;
        const preset_nodes_t& t_new_nodes = t_preset->get_nodes();
        for( preset_nodes_t::const_iterator t_node_it = t_new_nodes.begin(); t_node_it != t_new_nodes.end(); ++t_node_it )
        {
            std::stringstream t_nn_str;
            t_nn_str << t_stream_no << "_" << t_node_it->first;
            std::string t_node_name = t_nn_str.str();
            t_name_replacements[ t_node_it->first ] = t_node_name;

            LDEBUG( plog, "Creating node of type <" << t_node_it->second << "> called <" << t_node_name << ">" );
            node_builder* t_builder = scarab::factory< node_builder >::get_instance()->create( t_node_it->second );
            if( t_builder == nullptr )
            {
                LWARN( plog, "Cannot find binding for node type <" << t_node_it->second << ">" );
                return -1;
            }

            t_builder->name() = t_node_name;

            // setup the node config
            scarab::param_node t_node_config;
            // first get the node-specific config, if present (refer to it by its original name, not t_node_name)
            if( a_node->has( t_node_it->first ) ) t_node_config.merge( *a_node->node_at( t_node_it->first ) );
            // add stream-wide config data to the node config
            if( a_node->has( "device" ) ) t_node_config.add( "device", *a_node->node_at( "device" ) );
            // pass the configuration to the builder
            t_builder->configure( t_node_config );

            t_builder->set_daq_control( f_daq_control.lock() );
            t_stream.f_nodes.insert( stream_template::nodes_t::value_type( t_node_it->first, t_builder ) );
        }

        typedef node_config_preset::connections_t preset_conn_t;
        const preset_conn_t& t_new_connections = t_preset->get_connections();
        for( preset_conn_t::const_iterator t_conn_it = t_new_connections.begin(); t_conn_it != t_new_connections.end(); ++t_conn_it )
        {
            std::string t_connection( *t_conn_it );
            for( std::map< std::string, std::string >::const_iterator t_it = t_name_replacements.begin(); t_it != t_name_replacements.end(); ++t_it )
            {
                boost::replace_all( t_connection, t_it->first, t_it->second );
            }
            LDEBUG( plog, "Adding connection: " << t_connection );
            t_stream.f_connections.insert( t_connection );
        }

        // add the new stream to the vector of streams
        f_must_reset_midge = true;
        f_streams.push_back( t_stream );
        return f_streams.size() - 1;
    }

    void stream_manager::reset_midge()
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );

        f_must_reset_midge = true;

        std::unique_lock< std::mutex > t_midge_lock( f_midge_mutex );
        f_midge.reset( new midge::diptera() );

        for( streams_t::const_iterator t_stream_it = f_streams.begin(); t_stream_it != f_streams.end(); ++t_stream_it )
        {
            for( stream_template::nodes_t::const_iterator t_node_it = t_stream_it->f_nodes.begin(); t_node_it != t_stream_it->f_nodes.end(); ++t_node_it )
            {
                midge::node* t_new_node = t_node_it->second->build();

                try
                {
                    LINFO( plog, "Adding node <" << t_node_it->first << ">" );
                    f_midge->add( t_new_node );
                }
                catch( std::exception& e )
                {
                    delete t_new_node;
                    throw error() << "Unable to add processor <" << t_node_it->first << ">: " << e.what();
                }
            }

            // Then deal with connections
            for( stream_template::connections_t::const_iterator t_conn_it = t_stream_it->f_connections.begin(); t_conn_it != t_stream_it->f_connections.end(); ++t_conn_it )
            {
                try
                {
                    LINFO( plog, "Adding connection <" << *t_conn_it << ">" );
                    f_midge->join( *t_conn_it );
                }
                catch( std::exception& e )
                {
                    throw error() << "Unable to join nodes: " << e.what();
                }

                LINFO( plog, "Node connection made:  <" << *t_conn_it << ">" );
            }
        }

        f_must_reset_midge = false;
        return;
    }

    midge_package stream_manager::get_midge()
    {
        if( f_must_reset_midge )
        {
            reset_midge();
        }
        return midge_package( f_midge, f_midge_mutex );
    }

    void stream_manager::return_midge( midge_package&& a_midge )
    {
        midge_package t_returned( std::move( a_midge ) );
        t_returned.unlock();
        f_must_reset_midge = true;
        return;
    }

    std::string stream_manager::get_node_run_str() const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        std::string t_run_str;
        for( streams_t::const_iterator t_stream_it = f_streams.begin(); t_stream_it != f_streams.end(); ++t_stream_it )
        {
            stream_template::nodes_t::const_iterator t_node_it = t_stream_it->f_nodes.begin();
            t_run_str = t_node_it->first;
            for( ++t_node_it; t_node_it != t_stream_it->f_nodes.end(); ++t_node_it )
            {
                t_run_str += midge::diptera::separator() + t_node_it->first;
            }
        }
        return t_run_str;
    }

    bool stream_manager::is_in_use() const
    {
        LERROR( plog, "Checking if manager mutex is in use" );
        if( f_manager_mutex.try_lock() )
        {
            f_manager_mutex.unlock();
            return false;
        }
        else return true;
    }

} /* namespace psyllid */
