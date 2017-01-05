/*
 * node_manager.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "node_manager.hh"

#include "daq_control.hh"
#include "node_config_preset.hh"
#include "psyllid_error.hh"

#include "midge_error.hh"
#include "node.hh"

#include "factory.hh"
#include "logger.hh"
#include "param.hh"

#include <signal.h>

using std::string;

using scarab::param_array;
using scarab::param_node;
using scarab::param_value;

using midge::diptera;

using dripline::request_ptr_t;
using dripline::retcode_t;

namespace psyllid
{
    LOGGER( plog, "node_manager" );

    node_manager::node_manager( const param_node& a_master_config ) :
            control_access(),
            f_manager_mutex(),
            f_midge( new diptera() ),
            f_must_reset_midge( false ),
			f_midge_mutex(),
            f_nodes(),
            f_connections(),
            f_config( new param_node() )
    {
        f_config.reset( new param_node( a_master_config ) );
    }

    node_manager::~node_manager()
    {
        while( ! f_nodes.empty() )
        {
            delete f_nodes.begin()->second;
            f_nodes.erase( f_nodes.begin() );
        }
    }

    void node_manager::initialize()
    {
        // DAQ config is optional; defaults will work just fine
        if( f_config->has( "daq" ) )
        {
            param_node* t_daq_node = f_config->node_at( "daq" );
            if( t_daq_node != nullptr )
            {
                if( t_daq_node->has( "preset" ) )
                {
                    try
                    {
                        use_preset( t_daq_node->get_value( "preset" ) );
                    }
                    catch( error& e )
                    {
                        LERROR( plog, "Unable to apply DAQ preset: " << e.what() );
                        raise( SIGINT );
                    }
                }
            }
        }

    }

    void node_manager::use_preset( const std::string& a_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        f_must_reset_midge = true;

        node_config_preset* t_preset = scarab::factory< node_config_preset, const std::string& >::get_instance()->create( a_name, a_name );

        if( t_preset == nullptr )
        {
            throw error() << "Unable to create preset called <" << a_name << ">. The name may not be registered or there may be a typo.";
        }

        f_nodes.clear();
        f_connections.clear();

        typedef node_config_preset::nodes_t preset_nodes_t;
        const preset_nodes_t& t_new_nodes = t_preset->get_nodes();
        for( preset_nodes_t::const_iterator t_node_it = t_new_nodes.begin(); t_node_it != t_new_nodes.end(); ++t_node_it )
        {
            LDEBUG( plog, "Creating node of type <" << t_node_it->second << "> called <" << t_node_it->first << ">" );
            _add_node( t_node_it->second, t_node_it->first );
        }

        typedef node_config_preset::connections_t preset_conn_t;
        const preset_conn_t& t_new_connections = t_preset->get_connections();
        for( preset_conn_t::const_iterator t_conn_it = t_new_connections.begin(); t_conn_it != t_new_connections.end(); ++t_conn_it )
        {
            LDEBUG( plog, "Adding connection: " << *t_conn_it );
            _add_connection( *t_conn_it );
        }

        return;
    }

    void node_manager::configure_node( const std::string& a_node_name, const scarab::param_node& a_config )
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );
        try
        {
            LDEBUG( plog, "Configuring node <" << a_node_name << "> with:\n" << a_config );
            f_nodes.at( a_node_name )->configure( a_config );
            return;
        }
        catch( std::out_of_range& e )
        {
            throw error() << "Cannot configure node; node named <" << a_node_name << "> is not present";
        }
        return;
    }

    void node_manager::replace_node_config( const std::string& a_node_name, const scarab::param_node& a_config )
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );
        try
        {
            f_nodes.at( a_node_name )->replace_config( a_config );
            return;
        }
        catch( std::out_of_range& e )
        {
            throw error() << "Cannot replace node config; node named <" << a_node_name << "> is not present";
        }
        return;
    }

    void node_manager::dump_node_config( const std::string& a_node_name, scarab::param_node& a_config )
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );
        try
        {
            f_nodes.at( a_node_name )->dump_config( a_config );
            return;
        }
        catch( std::out_of_range& e )
        {
            throw error() << "Cannot dump node config; node named <" << a_node_name << "> is not present";
        }
        return;
    }

    void node_manager::reset_midge()
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );

        f_must_reset_midge = true;

        std::unique_lock< std::mutex > t_midge_lock( f_midge_mutex );
        f_midge.reset( new diptera() );

        for( nodes_t::const_iterator t_node_it = f_nodes.begin(); t_node_it != f_nodes.end(); ++t_node_it )
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
        for( connections_t::const_iterator t_conn_it = f_connections.begin(); t_conn_it != f_connections.end(); ++t_conn_it )
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

        f_must_reset_midge = false;
        return;
    }

    std::string node_manager::get_node_run_str() const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        nodes_t::const_iterator t_node_it = f_nodes.begin();
        std::string t_run_str( t_node_it->first );
        for( ++t_node_it; t_node_it != f_nodes.end(); ++t_node_it )
        {
            t_run_str += diptera::separator() + t_node_it->first;
        }
        return t_run_str;
    }

    midge_package node_manager::get_midge()
    {
        if( f_must_reset_midge )
        {
            reset_midge();
        }
        return midge_package( f_midge, f_midge_mutex );
    }

    void node_manager::return_midge( midge_package&& a_midge )
    {
        midge_package t_returned( std::move( a_midge ) );
        t_returned.unlock();
        f_must_reset_midge = true;
        return;
    }


    void node_manager::_add_node( const std::string& a_node_type, const std::string& a_node_name )
    {
        f_must_reset_midge = true;
        node_builder* t_builder = scarab::factory< node_builder >::get_instance()->create( a_node_type );
        if( t_builder == nullptr )
        {
            throw error() << "Cannot find binding for node type <" << a_node_type << ">";
        }
        t_builder->name() = a_node_name;
        if( f_config->has( a_node_name ) )
        {
            t_builder->configure( *f_config->node_at( a_node_name ) );
        }
        t_builder->set_daq_control( f_daq_control.lock() );
        f_nodes.insert( nodes_t::value_type( a_node_name, t_builder ) );
        return;
    }

    bool node_manager::is_in_use() const
    {
    	LERROR( plog, "Checking if manager mutex is in use" );
        if( f_manager_mutex.try_lock() )
        {
            f_manager_mutex.unlock();
            return false;
        }
        else return true;
    }



    bool node_manager::handle_apply_preset_request( const request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex, std::try_to_lock );
        if( ! t_lock.owns_lock() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error_no_resp, "Node manager is busy" );
        }

        const param_node& t_payload = a_request->get_payload();
        if( ! t_payload.has( "values" ) || t_payload.is_array() )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_value, "Values array not present or is not an array" );
        }
        string t_name( t_payload.array_at( "values" )->get_value( 0 ) );

        try
        {
            t_lock.release();
            f_manager_mutex.unlock();
            use_preset( t_name );
            return a_reply_pkg.send_reply( retcode_t::success, "Preset <" + t_name + " has been applied" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error, "Unable to apply preset <" + t_name + ">: " + e.what() );
        }
    }

    bool node_manager::handle_set_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex, std::try_to_lock );
        if( ! t_lock.owns_lock() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error_no_resp, "Node manager is busy" );
        }

        if( a_request->parsed_rks().empty() )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "RKS does not specify the node name" );
        }

        size_t t_rks_size = a_request->parsed_rks().size();

        string t_target_node = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        LDEBUG( plog, "Set node request RKS is <" << a_request->parsed_rks().to_string() << ">; size = " << a_request->parsed_rks().size() );

        if( t_rks_size == 1 )
        {
            // Replace full config for node
            LDEBUG( plog, "Replacing full config for node <" << t_target_node << ">" );

            const param_node& t_payload = a_request->get_payload();
            try
            {
                t_lock.release();
                f_manager_mutex.unlock();
                replace_node_config( t_target_node, t_payload );
                return a_reply_pkg.send_reply( retcode_t::success, "Replaced config for node <" + t_target_node + ">" );
            }
            catch( error& e )
            {
                return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
            }
        }
        else if( t_rks_size == 2 )
        {
            // Set a single configuration value

            string t_config_value_name = a_request->parsed_rks().front();
            a_request->parsed_rks().pop_front();

            const param_node& t_payload = a_request->get_payload();

            LDEBUG( plog, "Replacing value for configuration value <" << t_config_value_name << "> in node <" << t_target_node << ">; payload:\n" << t_payload );

            if( ! t_payload.has( "values" ) || t_payload.is_array() )
            {
                return a_reply_pkg.send_reply( retcode_t::message_error_invalid_value, "Values array not present or is not an array" );
            }

            param_node t_config;
            t_config.add( t_config_value_name, new param_value( t_payload.array_at( "values" )->get_value( 0 ) ) );

            try
            {
                t_lock.release();
                f_manager_mutex.unlock();
                configure_node( t_target_node, t_config );
                return a_reply_pkg.send_reply( retcode_t::success, "Set config value <" + t_config_value_name + "> for node <" + t_target_node + ">" );
                }
            catch( error& e )
            {
                return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
            }
       }

        return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "RKS is not formatted properly" );
    }

    bool node_manager::handle_get_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex, std::try_to_lock );
        if( ! t_lock.owns_lock() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error_no_resp, "Node manager is busy" );
        }

        if( a_request->parsed_rks().empty() )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "RKS does not specify the node name" );
        }

        string t_target_node = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        size_t t_rks_size = a_request->parsed_rks().size();
        if( t_rks_size == 1 )
        {
            // Get the full config for node
            LDEBUG( plog, "Getting full config for node <" << t_target_node << ">" );

            try
            {
                t_lock.release();
                f_manager_mutex.unlock();
                dump_node_config( t_target_node, a_request->get_payload() );
                return a_reply_pkg.send_reply( retcode_t::success, "Retrieved config for node <" + t_target_node + ">" );
            }
            catch( error& e )
            {
                return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
            }
        }
        else if( t_rks_size == 2 )
        {
            // Get a single configuration value

            string t_config_value_name = a_request->parsed_rks().front();
            a_request->parsed_rks().pop_front();

            try
            {
                param_node t_node_config;
                t_lock.release();
                f_manager_mutex.unlock();
                dump_node_config( t_target_node, t_node_config );

                if( ! t_node_config.has( t_config_value_name ) )
                {
                    return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "Did not find config value <" + t_config_value_name + "> in node <" + t_target_node + ">" );
                }

                param_array* t_values_array = new param_array();
                t_values_array->assign( 0, *t_node_config.value_at( t_config_value_name ) );
                a_request->get_payload().add( "values", t_values_array );

                return a_reply_pkg.send_reply( retcode_t::success, "Get config value <" + t_config_value_name + "> for node <" + t_target_node + ">" );
            }
            catch( error& e )
            {
                return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
            }
       }

        return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "RKS is not formatted properly" );
    }


} /* namespace psyllid */
