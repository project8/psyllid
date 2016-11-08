/*
 * node_manager.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_MANAGER_HH_
#define PSYLLID_NODE_MANAGER_HH_

#include "locked_resource.hh"
#include "node_builder.hh"

#include "diptera.hh"

#include "hub.hh"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

namespace scarab
{
    class param_node;
}

namespace psyllid
{
    class node_manager;
    typedef locked_resource< midge::diptera, node_manager > midge_package;

    class daq_control;

    class node_manager : public control_access
    {
        public:
            typedef std::shared_ptr< midge::diptera > midge_ptr_t;

        public:
            node_manager( const scarab::param_node& a_master_node );
            virtual ~node_manager();

            void initialize(); // will setup preset nodes if specified; use only after f_daq_control is set

            //bool configure( const scarab::param_node* a_node );
            void use_preset( const std::string& a_name ); // can thrown psyllid::error

            void add_node( const std::string& a_node_type, const std::string& a_node_name );
            void remove_node( const std::string& a_node_name );
            void clear_nodes();

            void configure_node( const std::string& a_node_name, const scarab::param_node& a_config );
            void replace_node_config( const std::string& a_node_name, const scarab::param_node& a_config );
            void dump_node_config( const std::string& a_node_name, scarab::param_node& a_config );

            void add_connection( const std::string& a_connection );
            void remove_connection( const std::string& a_connection );
            void clear_connections();

            void reset_midge(); // throws psyllid::error in the event of an error configuring midge
            bool must_reset_midge() const;

            midge_package get_midge();
            void return_midge( midge_package&& a_midge );

            std::string get_node_run_str() const;

            bool is_in_use() const;

        public:
            bool handle_apply_preset_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_set_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_get_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

        private:
            // not thread-safe
            void _add_node( const std::string& a_node_type, const std::string& a_node_name );
            // not thread-safe
            void _add_connection( const std::string& a_connection );

            mutable std::mutex f_manager_mutex;

            midge_ptr_t f_midge;
            bool f_must_reset_midge;
            mutable std::mutex f_midge_mutex;

            typedef std::map< std::string, node_builder* > nodes_t;
            typedef std::set< std::string > connections_t;

            nodes_t f_nodes;
            connections_t f_connections;

            std::unique_ptr< scarab::param_node > f_config;
    };

    inline void node_manager::add_node( const std::string& a_node_type, const std::string& a_node_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        _add_node( a_node_type, a_node_name );
        return;
    }

    inline void node_manager::remove_node( const std::string& a_node_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;
        f_nodes.erase( a_node_name );
        return;
    }

    inline void node_manager::clear_nodes()
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_nodes.clear();
        f_must_reset_midge = true;
        return;
    }

    inline void node_manager::add_connection( const std::string& a_connection )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        _add_connection( a_connection );
        return;
    }

    inline void node_manager::remove_connection( const std::string& a_connection )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;
        f_connections.erase( a_connection );
    }

    inline void node_manager::clear_connections()
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_connections.clear();
        return;
    }

    inline bool node_manager::must_reset_midge() const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        return f_must_reset_midge;
    }

    inline void node_manager::_add_connection( const std::string& a_connection )
    {
        f_must_reset_midge = true;
        f_connections.insert( a_connection );
        return;
    }


} /* namespace psyllid */

#endif /* PSYLLID_NODE_MANAGER_HH_ */
