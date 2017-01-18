/*
 * stream_manager.cc
 *
 *  Created on: Jan 5, 2017
 *      Author: obla999
 */

#include "stream_manager.hh"

#include "node_builder.hh"
#include "psyllid_error.hh"
#include "stream_preset.hh"

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
            for( stream_template::nodes_t::iterator t_node_it = t_stream_it->second.f_nodes.begin(); t_node_it != t_stream_it->second.f_nodes.end(); ++t_node_it )
            {
                delete t_node_it->second;
                t_node_it->second = nullptr;
            }
        }
    }

    bool stream_manager::initialize( const scarab::param_node& a_config )
    {
        for( scarab::param_node::const_iterator t_str_conf_it = a_config.begin(); t_str_conf_it != a_config.end(); ++t_str_conf_it )
        {
            if( ! t_str_conf_it->second->is_node() )
            {
                LERROR( plog, "Invalid stream configuration" );
                return false;
            }
            if( ! add_stream( t_str_conf_it->first, &(t_str_conf_it->second->as_node()) ) )
            {
                LERROR( plog, "Something went wrong while adding a stream" );
                return false;
            }
        }
        return true;
    }

    bool stream_manager::add_stream( const std::string& a_name, const scarab::param_node* a_node )
    {
        // do not need to lock the mutex here because we're not doing anything to the stream_manager until inside _add_stream()
        try
        {
            _add_stream( a_name, a_node );
            return true;
        }
        catch( error& e )
        {
            LERROR( plog, e.what() );
            return false;
        }
    }

    void stream_manager::remove_stream( const std::string& a_name )
    {
        // do not need to lock the mutex here because we're not doing anything to the stream_manager until inside _remove_stream()
        try
        {
            return _remove_stream( a_name );
        }
        catch( error& e )
        {
            LWARN( plog, e.what() );
            return;
        }
    }

    bool stream_manager::configure_node( const std::string& a_stream_name, const std::string& a_node_name, const scarab::param_node& a_config )
    {
        try
        {
            _configure_node( a_stream_name, a_node_name, a_config );
            return true;
        }
        catch( error& e )
        {
            LWARN( plog, "Unable to configure node <" << a_stream_name << "." << a_node_name << ">: " << e.what() );
            return false;
        }
    }

    bool stream_manager::dump_node_config( const std::string& a_stream_name, const std::string& a_node_name, scarab::param_node& a_config ) const
    {
        try
        {
            _dump_node_config( a_stream_name, a_node_name, a_config );
            return true;
        }
        catch( error& e )
        {
            LWARN( plog, "Unable to dump node config <" << a_stream_name << "." << a_node_name << ">: " << e.what() );
            return false;
        }
    }

    void stream_manager::_configure_node( const std::string& a_stream_name, const std::string& a_node_name, const scarab::param_node& a_config )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        streams_t::iterator t_stream_it = f_streams.find( a_stream_name );
        if( t_stream_it == f_streams.end() )
        {
            throw error() << "Did not find stream <" << a_stream_name << ">";
        }

        stream_template::nodes_t::iterator t_node_it = t_stream_it->second.f_nodes.find( a_node_name );
        if( t_node_it == t_stream_it->second.f_nodes.end() )
        {
            throw error() << "Did not find node <" << a_node_name << "> in stream <" << a_stream_name << ">";
        }

        t_node_it->second->configure( a_config );

        return;
    }

    void stream_manager::_dump_node_config( const std::string& a_stream_name, const std::string& a_node_name, scarab::param_node& a_config ) const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        streams_t::const_iterator t_stream_it = f_streams.find( a_stream_name );
        if( t_stream_it == f_streams.end() )
        {
            throw error() << "Did not find stream <" << a_stream_name << ">";
        }

        stream_template::nodes_t::const_iterator t_node_it = t_stream_it->second.f_nodes.find( a_node_name );
        if( t_node_it == t_stream_it->second.f_nodes.end() )
        {
            throw error() << "Did not find node <" << a_node_name << "> in stream <" << a_stream_name << ">";
        }

        t_node_it->second->dump_config( a_config );

        return;

    }


    void stream_manager::_add_stream( const std::string& a_name, const scarab::param_node* a_node )
    {
        // do not need to lock the mutex here because we're not doing anything to the stream_manager until inside _add_stream( string, param_node )

        if( a_node == nullptr )
        {
            throw error() << "Null config received";
        }

        const scarab::param* t_preset_param = a_node->at( "preset" );

        if( t_preset_param == nullptr )
        {
            throw error() << "No preset specified";
        }
        else if( t_preset_param->is_node() )
        {
            const scarab::param_node* t_preset_param_node = &t_preset_param->as_node();
            if( ! node_config_runtime_preset::add_preset( t_preset_param_node ) )
            {
                throw error() << "Runtime preset could not be added";
            }
            // "name" is guaranteed to be there by the successful completion of node_config_runtime_preset::add_preset
            try
            {
                return _add_stream( a_name, t_preset_param_node->get_value( "type" ), a_node );
            }
            catch( error& e )
            {
                throw( e );
            }
        }
        else if( t_preset_param->is_value() )
        {
            try
            {
                return _add_stream( a_name, t_preset_param->as_value().as_string(), a_node );
            }
            catch( error& e )
            {
                throw( e );
            }
        }
        else
        {
            throw error() << "Invalid preset specification";
        }
    }

    void stream_manager::_add_stream( const std::string& a_name, const std::string& a_type, const scarab::param_node* a_node )
    {
        // do not need to lock the mutex here because we're not doing anything to the stream_manager until later

        stream_preset* t_preset = scarab::factory< stream_preset, const std::string& >::get_instance()->create( a_type, a_type );

        if( t_preset == nullptr )
        {
            throw error() << "Unable to create preset called <" << a_name << "> of type <" << a_type << ">. The type may not be registered or there may be a typo.";
        }

        // lock the mutex here so that we know which stream name this will be if it succeeds
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        if( f_streams.find( a_name ) != f_streams.end() )
        {
            throw error() << "Already have a stream called <" << a_name << ">";
        }

        LINFO( plog, "Preparing stream <" << a_name << ">");

        stream_template t_stream;

        typedef stream_preset::nodes_t preset_nodes_t;
        const preset_nodes_t& t_new_nodes = t_preset->get_nodes();
        std::map< std::string, std::string > t_name_replacements;
        for( preset_nodes_t::const_iterator t_node_it = t_new_nodes.begin(); t_node_it != t_new_nodes.end(); ++t_node_it )
        {
            std::stringstream t_nn_str;
            t_nn_str << a_name << "_" << t_node_it->first;
            std::string t_node_name = t_nn_str.str();
            t_name_replacements[ t_node_it->first ] = t_node_name;

            LDEBUG( plog, "Creating node of type <" << t_node_it->second << "> called <" << t_node_name << ">" );
            node_builder* t_builder = scarab::factory< node_builder >::get_instance()->create( t_node_it->second );
            if( t_builder == nullptr )
            {
                throw error() << "Cannot find binding for node type <" << t_node_it->second << ">";
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

        typedef stream_preset::connections_t preset_conn_t;
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
        f_streams.insert( streams_t::value_type( a_name, t_stream ) );
        return;
    }

    void stream_manager::_remove_stream( const std::string& a_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        // delete node_builder objects
        streams_t::iterator t_to_erase = f_streams.find( a_name );
        if( t_to_erase == f_streams.end() )
        {
            throw error() << "Stream <" << a_name << "> does not exist";
        }

        f_must_reset_midge = true;

        for( stream_template::nodes_t::iterator t_node_it = t_to_erase->second.f_nodes.begin(); t_node_it != t_to_erase->second.f_nodes.end(); ++t_node_it )
        {
            delete t_node_it->second;
            t_node_it->second = nullptr;
        }

        f_streams.erase( t_to_erase );

        return;
    }



    void stream_manager::reset_midge()
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );

        if( f_streams.empty() )
        {
            throw error() << "No streams have been setup";
        }

        f_must_reset_midge = true;

        std::unique_lock< std::mutex > t_midge_lock( f_midge_mutex );
        f_midge.reset( new midge::diptera() );

        for( streams_t::const_iterator t_stream_it = f_streams.begin(); t_stream_it != f_streams.end(); ++t_stream_it )
        {
            for( stream_template::nodes_t::const_iterator t_node_it = t_stream_it->second.f_nodes.begin(); t_node_it != t_stream_it->second.f_nodes.end(); ++t_node_it )
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
            for( stream_template::connections_t::const_iterator t_conn_it = t_stream_it->second.f_connections.begin(); t_conn_it != t_stream_it->second.f_connections.end(); ++t_conn_it )
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
            stream_template::nodes_t::const_iterator t_node_it = t_stream_it->second.f_nodes.begin();
            t_run_str = t_stream_it->first + "_" + t_node_it->first;
            for( ++t_node_it; t_node_it != t_stream_it->second.f_nodes.end(); ++t_node_it )
            {
                t_run_str += midge::diptera::separator() + t_stream_it->first + "_" + t_node_it->first;
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

    bool stream_manager::handle_add_stream_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( ! a_request->get_payload().has( "name" ) || ! a_request->get_payload().has( "config" ) )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Add-stream request is missing either \"name\" or \"config\"" );
        }

        try
        {
            add_stream( a_request->get_payload().get_value( "name" ), a_request->get_payload().node_at( "config" ) );
        }
        catch( std::exception& e )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::warning_no_action_taken, e.what() );
        }

        return a_reply_pkg.send_reply( dripline::retcode_t::success, "Stream " + a_request->get_payload().get_value( "name" ) + " has been added" );
    }

    bool stream_manager::handle_remove_stream_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( ! a_request->get_payload().has( "stream" ) || ! a_request->get_payload().at( "stream" )->is_value() )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "No stream specified for removal" );
        }

        try
        {
            _remove_stream( a_request->get_payload().get_value( "stream" ) );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::warning_no_action_taken, e.what() );
        }

        return a_reply_pkg.send_reply( dripline::retcode_t::success, "Stream " + a_request->get_payload().get_value( "stream" ) + " has been removed" );
    }

    bool stream_manager::handle_configure_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( a_request->parsed_rks().size() < 2 )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "RKS is improperly formatted: [queue].node-config.[stream].[node] or [queue].node-config.[stream].[node].[parameter]" );
        }

        //size_t t_rks_size = a_request->parsed_rks().size();

        std::string t_target_stream = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        std::string t_target_node = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        if( a_request->parsed_rks().empty() )
        {
            // payload should be a map of all parameters to be set
            LDEBUG( plog, "Performing node config for multiple values in stream <" << t_target_stream << "> and node <" << t_target_node << ">" );

            if( a_request->get_payload().empty() )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Unable to perform node-config request: payload is empty" );
            }

            try
            {
                _configure_node( t_target_stream, t_target_node, a_request->get_payload() );
                a_reply_pkg.f_payload.merge( a_request->get_payload() );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform node-config request: ") + e.what() );
            }
        }
        else
        {
            // payload should be values array with a single entry for the particular parameter to be set
            LDEBUG( plog, "Performing node config for a single value in stream <" << t_target_stream << "> and node <" << t_target_node << ">" );

            if( ! a_request->get_payload().has( "values" ) )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Unable to perform node-config (single value): values array is missing" );
            }
            const scarab::param_array* t_values_array = a_request->get_payload().array_at( "values" );
            if( t_values_array == nullptr || t_values_array->empty() || ! (*t_values_array)[0].is_value() )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::message_error_bad_payload, "Unable to perform node-config (single value): \"values\" is not an array, or the array is empty, or the first element in the array is not a value" );
            }

            scarab::param_node t_param_to_set;
            t_param_to_set.add( a_request->parsed_rks().front(), new scarab::param_value( (*t_values_array)[0].as_value() ) );

            try
            {
                _configure_node( t_target_stream, t_target_node, t_param_to_set );
                a_reply_pkg.f_payload.merge( t_param_to_set );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform node-config request (single value): ") + e.what() );
            }
        }

        LDEBUG( plog, "Node-config was successful" );
        return a_reply_pkg.send_reply( dripline::retcode_t::success, "Performed node-config" );
    }

    bool stream_manager::handle_dump_config_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        if( a_request->parsed_rks().size() < 2 )
        {
            return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "RKS is improperly formatted: [queue].node-config.[stream].[node] or [queue].node-config.[stream].[node].[parameter]" );
        }

        //size_t t_rks_size = a_request->parsed_rks().size();

        std::string t_target_stream = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        std::string t_target_node = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        if( a_request->parsed_rks().empty() )
        {
            // getting full node configuration
            LDEBUG( plog, "Getting full node config for stream <" << t_target_stream << "> and node <" << t_target_node << ">" );

            try
            {
                _dump_node_config( t_target_stream, t_target_node, a_reply_pkg.f_payload );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to perform get-node-config request: ") + e.what() );
            }
        }
        else
        {
            // getting value for a single parameter
            LDEBUG( plog, "Getting value for a single parameter in stream <" << t_target_stream << "> and node <" << t_target_node << ">" );

            std::string t_param_to_get = a_request->parsed_rks().front();

            try
            {
                scarab::param_node t_param_dump;
                _dump_node_config( t_target_stream, t_target_node, t_param_dump );
                if( ! t_param_dump.has( t_param_to_get ) )
                {
                    return a_reply_pkg.send_reply( dripline::retcode_t::message_error_invalid_key, "Unable to get node parameter: cannot find parameter <" + t_param_to_get + ">" );
                }
                a_reply_pkg.f_payload.add( t_param_to_get, new scarab::param_value( *t_param_dump.value_at( t_param_to_get ) ) );
            }
            catch( std::exception& e )
            {
                return a_reply_pkg.send_reply( dripline::retcode_t::device_error, std::string("Unable to get node parameter (single value): ") + e.what() );
            }
        }

        LDEBUG( plog, "Get-node-config was successful" );
        return a_reply_pkg.send_reply( dripline::retcode_t::success, "Performed get-node-config" );
    }

} /* namespace psyllid */
