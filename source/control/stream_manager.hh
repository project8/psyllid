/*
 * stream_manager.hh
 *
 *  Created on: Jan 5, 2017
 *      Author: obla999
 */

#ifndef PSYLLID_STREAM_MANAGER_HH_
#define PSYLLID_STREAM_MANAGER_HH_

#include "locked_resource.hh"

#include "diptera.hh"

#include "hub.hh"

#include "param.hh"

#include <map>
#include <memory>
#include <mutex>

namespace psyllid
{
    class stream_manager;
    typedef locked_resource< midge::diptera, stream_manager > midge_package;

    class node_binding;
    // the node_binding is owned by this map; the node is not
    typedef std::map< std::string, std::pair< node_binding*, midge::node* > > active_node_bindings;

    class node_builder;

    class stream_manager
    {
        public:
            struct stream_template
            {
                scarab::param_node f_device_config;

                typedef std::map< std::string, node_builder* > nodes_t;
                typedef std::set< std::string > connections_t;

                nodes_t f_nodes;
                connections_t f_connections;

                //std::string f_run_string;
            };

            typedef std::shared_ptr< midge::diptera > midge_ptr_t;

        public:
            stream_manager();
            virtual ~stream_manager();

            bool initialize( const scarab::param_node& a_config );

        public:
            bool add_stream( const std::string& a_name, const scarab::param_node* a_node );
            const stream_template* get_stream( const std::string& a_name ) const;
            void remove_stream( const std::string& a_name );

            bool configure_node( const std::string& a_stream_name, const std::string& a_node_name, const scarab::param_node& a_config );
            bool dump_node_config( const std::string& a_stream_name, const std::string& a_node_name, scarab::param_node& a_config ) const;

        public:
            void reset_midge(); // throws psyllid::error in the event of an error configuring midge
            bool must_reset_midge() const;

            midge_package get_midge();
            void return_midge( midge_package&& a_midge );

            active_node_bindings* get_node_bindings();

            std::string get_node_run_str() const;

            bool is_in_use() const;

        public:
            bool handle_add_stream_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_remove_stream_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_configure_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_dump_config_node_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

        private:
            void _add_stream( const std::string& a_name, const scarab::param_node* a_node );
            void _add_stream( const std::string& a_name, const std::string& a_type, const scarab::param_node* a_node );
            void _remove_stream( const std::string& a_name );

            void _configure_node( const std::string& a_stream_name, const std::string& a_node_name, const scarab::param_node& a_config );
            void _dump_node_config( const std::string& a_stream_name, const std::string& a_node_name, scarab::param_node& a_config ) const;

            void clear_node_bindings();

            typedef std::map< std::string, stream_template > streams_t;
            streams_t f_streams;

            mutable std::mutex f_manager_mutex;

            midge_ptr_t f_midge;
            active_node_bindings f_node_bindings;
            bool f_must_reset_midge;
            mutable std::mutex f_midge_mutex;
    };


    inline const stream_manager::stream_template* stream_manager::get_stream( const std::string& a_name ) const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        streams_t::const_iterator t_stream = f_streams.find( a_name );
        if( t_stream == f_streams.end() ) return nullptr;
        return &(t_stream->second);
    }

    inline bool stream_manager::must_reset_midge() const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        return f_must_reset_midge;
    }

    inline active_node_bindings* stream_manager::get_node_bindings()
    {
        return &f_node_bindings;
    }


} /* namespace psyllid */

#endif /* PSYLLID_STREAM_MANAGER_HH_ */
