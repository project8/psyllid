/*
 * node_manager.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_MANAGER_HH_
#define PSYLLID_NODE_MANAGER_HH_

#include "diptera.hh"

#include "hub.hh"

#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace scarab
{
    class param_node;
}

namespace psyllid
{

    class midge_package
    {
        public:
            typedef std::shared_ptr< midge::diptera > midge_ptr_t;

            midge_package() :
                f_midge(),
                f_lock()
            {}
            midge_package( const midge_package& ) = delete;
            midge_package( midge_package&& a_orig ) :
                f_midge( std::move( a_orig.f_midge ) ),
                f_lock( std::move( a_orig.f_lock ) )
            {}
            ~midge_package() {}

            midge_package& operator=( const midge_package& ) = delete;
            midge_package& operator=( midge_package&& a_rhs )
            {
                f_midge = std::move( a_rhs.f_midge );
                a_rhs.f_midge.reset();
                f_lock = std::move( a_rhs.f_lock );
                a_rhs.f_lock.release();
                return *this;
            }

            midge::diptera* operator->() const { return f_midge.get(); }

        private:
            friend class node_manager;
            midge_package( midge_ptr_t a_midge, std::mutex& a_mutex ) :
                f_midge( a_midge ),
                f_lock( a_mutex )
            {}

            midge_ptr_t f_midge;
            std::unique_lock< std::mutex > f_lock;
    };

    class node_manager
    {
        public:
            typedef std::shared_ptr< midge::diptera > midge_ptr_t;

        public:
            node_manager();
            virtual ~node_manager();

            //bool configure( const scarab::param_node* a_node );
            void use_preset( const std::string& a_name );

            void add_node( const std::string& a_node_type, const std::string& a_node_name );
            void remove_node( const std::string& a_node_name );

            void add_connection( const std::string& a_connection );
            void remove_connection( const std::string& a_connection );

            void reset_midge(); // throws psyllid::error in the event of an error configuring midge
            bool must_reset_midge() const;

            midge_package get_midge() const;

            std::string get_node_run_str() const;

        public:
            bool handle_apply_preset_request( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg );

            bool handle_get_node_config_request( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg );

        private:
            mutable std::mutex f_manager_mutex;

            midge_ptr_t f_midge;
            bool f_must_reset_midge;
            mutable std::mutex f_midge_mutex;

            typedef std::map< std::string, std::string > nodes_t;
            typedef std::set< std::string > connections_t;

            nodes_t f_nodes;
            connections_t f_connections;
    };

    inline void node_manager::add_node( const std::string& a_node_type, const std::string& a_node_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;
        f_nodes.insert( nodes_t::value_type( a_node_name, a_node_type ) );
        return;
    }

    inline void node_manager::remove_node( const std::string& a_node_name )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;
        f_nodes.erase( a_node_name );
        return;
    }

    inline void node_manager::add_connection( const std::string& a_connection )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;
        f_connections.insert( a_connection );
    }

    inline void node_manager::remove_connection( const std::string& a_connection )
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        f_must_reset_midge = true;
        f_connections.erase( a_connection );
    }

    inline bool node_manager::must_reset_midge() const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        return f_must_reset_midge;
    }

} /* namespace psyllid */

#endif /* PSYLLID_NODE_MANAGER_HH_ */
