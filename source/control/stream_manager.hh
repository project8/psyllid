/*
 * stream_manager.hh
 *
 *  Created on: Jan 5, 2017
 *      Author: obla999
 */

#ifndef PSYLLID_STREAM_MANAGER_HH_
#define PSYLLID_STREAM_MANAGER_HH_

#include "control_access.hh"

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

    class node_builder;

    class stream_manager : public control_access
    {
        public:
            struct stream_template
            {
                scarab::param_node f_device_config;

                typedef std::map< std::string, node_builder* > nodes_t;
                typedef std::set< std::string > connections_t;

                nodes_t f_nodes;
                connections_t f_connections;

                std::string f_run_string;
            };

            typedef std::shared_ptr< midge::diptera > midge_ptr_t;

        public:
            stream_manager();
            virtual ~stream_manager();

            bool initialize( const scarab::param_array& a_config );

        public:
            int add_stream( const scarab::param_node* a_node );

            const stream_template* get_stream( unsigned a_stream_no ) const;

            void remove_stream( unsigned a_stream_no );

        public:
            void reset_midge(); // throws psyllid::error in the event of an error configuring midge
            bool must_reset_midge() const;

            midge_package get_midge();
            void return_midge( midge_package&& a_midge );

            std::string get_node_run_str() const;

            bool is_in_use() const;

        public:
            bool handle_add_stream_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_remove_stream_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );


        private:
            int _add_stream( const scarab::param_node* a_node );
            int _add_stream( const std::string& a_name, const scarab::param_node* a_node );
            void _remove_stream( unsigned a_stream_no );

            typedef std::map< unsigned, stream_template > streams_t;
            streams_t f_streams;
            unsigned f_stream_counter;

            mutable std::mutex f_manager_mutex;

            midge_ptr_t f_midge;
            bool f_must_reset_midge;
            mutable std::mutex f_midge_mutex;
    };


    inline const stream_manager::stream_template* stream_manager::get_stream( unsigned a_stream_no ) const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        if( a_stream_no >= f_streams.size() ) return nullptr;
        return &f_streams.at( a_stream_no );
    }

    inline bool stream_manager::must_reset_midge() const
    {
        std::unique_lock< std::mutex > t_lock( f_manager_mutex );
        return f_must_reset_midge;
    }


} /* namespace psyllid */

#endif /* PSYLLID_STREAM_MANAGER_HH_ */
