/*
 * mt_run_server.hh
 *
 *  Created on: May 6, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_RUN_SERVER_HH_
#define PSYLLID_RUN_SERVER_HH_

//#include "mt_version.hh"

#include "hub.hh"

#include "param.hh"

#include "cancelable.hh"

#include <atomic>
#include <mutex>

namespace scarab
{
    class version_semantic;
}

namespace psyllid
{
    class daq_control;
    class request_receiver;
    class stream_manager;

    /*!
     @class run_server
     @author N. S. Oblath

     @brief Sets up daq_control, strea_manager and request_receiver. Registers request handles.

     @details
     A run_server instance is created by the psyllid executable. The executable calls run_server.execute() and waits for it's return.
     When told to execute the run_server instance creates new instances of  daq_control, stream_manager and request_receiver.
     run_server adds set, get and cmd request handlers by registering handlers in the request_receiver.
     Then it executes daq_control and request_receiver in 2 separate threads.
     run_server.execute() only returns when all threads were joined.
     */
    class run_server : public scarab::cancelable
    {
        public:
            run_server( const scarab::param_node& a_node, std::shared_ptr< scarab::version_semantic > a_version );
            virtual ~run_server();

            void execute();

            void quit_server();

            int get_return() const;

            const scarab::param_node& get_config() const;

            bool handle_get_server_status_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_stop_all_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_quit_server_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

        private:
            virtual void do_cancellation();

            scarab::param_node f_config;
            const std::shared_ptr< scarab::version_semantic > f_version;

            int f_return;

            // component pointers for asynchronous access
            std::shared_ptr< request_receiver > f_request_receiver;
            //std::shared_ptr< daq_worker > f_daq_worker;
            std::shared_ptr< daq_control > f_daq_control;
            std::shared_ptr< stream_manager > f_stream_manager;

            std::mutex f_component_mutex;

        public:
            enum status
            {
                k_initialized = 0,
                k_starting = 1,
                k_running = 5,
                k_done = 10,
                k_error = 100
            };

            static std::string interpret_status( status a_status );

            status get_status() const;
            void set_status( status a_status );

        private:
            std::atomic< status > f_status;

    };

    inline int run_server::get_return() const
    {
        return f_return;
    }

    inline const scarab::param_node& run_server::get_config() const
    {
        return f_config;
    }

    inline run_server::status run_server::get_status() const
    {
        return f_status.load();
    }

    inline void run_server::set_status( status a_status )
    {
        f_status.store( a_status );
        return;
    }

} /* namespace mantis */

#endif /* SERVER_MT_RUN_SERVER_HH_ */
