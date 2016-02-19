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
#include "node.hh"

#include "factory.hh"
#include "logger.hh"
#include "param.hh"

using std::string;

using scarab::param_array;
using scarab::param_node;
using scarab::param_value;

using midge::diptera;

using dripline::request_ptr_t;
using dripline::hub;
using dripline::retcode_t;

namespace psyllid
{
    LOGGER( plog, "node_manager" );

    node_manager::node_manager() :
            f_manager_mutex(),
            f_midge( new diptera() ),
            f_must_reset_midge( false ),
            f_nodes(),
            f_connections()
    {
    }

    node_manager::~node_manager()
    {
        while( ! f_nodes.empty() )
        {
            delete f_nodes.begin()->second;
            f_nodes.erase( f_nodes.begin() );
        }
    }

    void node_manager::use_preset( const std::string& a_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );

        f_must_reset_midge = true;

        node_config_preset* t_preset = scarab::factory< node_config_preset >::get_instance()->create( a_name );

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
            _add_node( t_node_it->second, t_node_it->first );
        }

        typedef node_config_preset::connections_t preset_conn_t;
        const preset_conn_t& t_new_connections = t_preset->get_connections();
        for( preset_conn_t::const_iterator t_conn_it = t_new_connections.begin(); t_conn_it != t_new_connections.end(); ++t_conn_it )
        {
            _add_connection( *t_conn_it );
        }

        return;
    }

    void node_manager::configure_node( const std::string& a_node_name, const scarab::param_node& a_config )
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );
        try
        {
            f_nodes.at( a_node_name )->configure( a_config );
            return;
        }
        catch( std::out_of_range& e )
        {
            throw error() << "Cannot configure node; node named <" << a_node_name << "> is not present";
        }
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
    }

    void node_manager::reset_midge()
    {
        std::unique_lock< std::mutex > t_mgr_lock( f_manager_mutex );

        f_must_reset_midge = true;

        f_midge.reset( new diptera() );

        for( nodes_t::const_iterator t_node_it = f_nodes.begin(); t_node_it != f_nodes.end(); ++t_node_it )
        {
            midge::node* t_new_node = t_node_it->second->build();

            try
            {
                f_midge->add( t_new_node );
                INFO( plog, "Added node <" << t_node_it->first << ">" );
            }
            catch( midge::error& e )
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
        return midge_package( f_midge, f_manager_mutex );
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
        f_nodes.insert( nodes_t::value_type( a_node_name, t_builder ) );
        return;
    }

    bool node_manager::is_in_use() const
    {
        if( f_manager_mutex.try_lock() )
        {
            return false;
            f_manager_mutex.unlock();
        }
        else return true;
    }



    bool node_manager::handle_apply_preset_request( const request_ptr_t a_request, hub::reply_package& a_reply_pkg )
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

    bool node_manager::handle_set_node_config_value_request( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex, std::try_to_lock );
        if( ! t_lock.owns_lock() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error_no_resp, "Node manager is busy" );
        }

        if( a_request->parsed_rks().size() != 2 )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "RKS is not formatted properly to specify a target node and configuration item" );
        }

        string t_target_node = a_request->parsed_rks().front();
        a_request->parsed_rks().pop();

        const param_node& t_payload = a_request->get_payload();
        if( ! t_payload.has( "values" ) || t_payload.is_array() )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_value, "Values array not present or is not an array" );
        }

        param_node t_config;
        t_config.add( a_request->parsed_rks().front(), new param_value( t_payload.array_at( "values" )->value_at( 0 ) ) );

        try
        {
            t_lock.release();
            f_manager_mutex.unlock();
            configure_node( t_target_node, t_config );
            return a_reply_pkg.send_reply( retcode_t::success, "Node <" + a_request->parsed_rks().front() + "> has been configured" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
        }
    }

    bool node_manager::handle_get_node_config_request( const request_ptr_t a_request, hub::reply_package& a_reply_pkg )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex, std::try_to_lock );
        if( ! t_lock.owns_lock() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error_no_resp, "Node manager is busy" );
        }

        if( a_request->parsed_rks().empty() )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "No target node provided in the RKS" );
        }

        try
        {
            t_lock.release();
            f_manager_mutex.unlock();
            dump_node_config( a_request->parsed_rks().front(), a_reply_pkg.f_payload );
            return a_reply_pkg.send_reply( retcode_t::success, "Node <" + a_request->parsed_rks().front() + "> configuration retrieved" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
        }
    }

    bool node_manager::handle_replace_node_config_request( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex, std::try_to_lock );
        if( ! t_lock.owns_lock() )
        {
            return a_reply_pkg.send_reply( retcode_t::device_error_no_resp, "Node manager is busy" );
        }

        if( a_request->parsed_rks().empty() )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, "No target node provided in the RKS" );
        }

        try
        {
            t_lock.release();
            f_manager_mutex.unlock();
            replace_node_config( a_request->parsed_rks().front(), a_request->get_payload() );
            return a_reply_pkg.send_reply( retcode_t::success, "Node <" + a_request->parsed_rks().front() + "> has been configured" );
        }
        catch( error& e )
        {
            return a_reply_pkg.send_reply( retcode_t::message_error_invalid_key, e.what() );
        }
    }


} /* namespace psyllid */
