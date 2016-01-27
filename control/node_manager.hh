/*
 * node_manager.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_MANAGER_HH_
#define PSYLLID_NODE_MANAGER_HH_

#include "diptera.hh"

#include <map>
#include <set>

namespace scarab
{
    class param_node;
}

namespace psyllid
{

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
            midge_ptr_t get_midge() const;

            std::string get_node_run_str() const;

        private:
            midge_ptr_t f_midge;
            bool f_must_reset_midge;

            typedef std::map< std::string, std::string > nodes_t;
            typedef std::set< std::string > connections_t;

            nodes_t f_nodes;
            connections_t f_connections;
    };

    inline void node_manager::add_node( const std::string& a_node_type, const std::string& a_node_name )
    {
        f_must_reset_midge = true;
        f_nodes.insert( nodes_t::value_type( a_node_name, a_node_type ) );
        return;
    }

    inline void node_manager::remove_node( const std::string& a_node_name )
    {
        f_must_reset_midge = true;
        f_nodes.erase( a_node_name );
        return;
    }

    inline void node_manager::add_connection( const std::string& a_connection )
    {
        f_must_reset_midge = true;
        f_connections.insert( a_connection );
    }

    inline void node_manager::remove_connection( const std::string& a_connection )
    {
        f_must_reset_midge = true;
        f_connections.erase( a_connection );
    }

    inline bool node_manager::must_reset_midge() const
    {
        return f_must_reset_midge;
    }

    inline node_manager::midge_ptr_t node_manager::get_midge() const
    {
        return f_midge;
    }

} /* namespace psyllid */

#endif /* PSYLLID_NODE_MANAGER_HH_ */
