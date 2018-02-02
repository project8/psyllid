#ifndef PSYLLID_REQUEST_RECEIVER_HH_
#define PSYLLID_REQUEST_RECEIVER_HH_

#include "hub.hh"

#include "param.hh"

#include "cancelable.hh"

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

namespace psyllid
{
    class run_server;
	/*!
     @class request_receiver
     @author N. S. Oblath

     @brief Receives request from a amqp broker.

     @details
     A request_receiver instance is initialized by run_server.
     request_receiver holds maps for set, get, cmd and run requests.
     When a request is received the handle_function registered with this request gets called.
     The registration of requests and functions is done in dripline::hub.
     */
    class request_receiver : public dripline::hub, public scarab::cancelable
    {
        private:
            typedef std::function< bool( const dripline::request_ptr_t, dripline::reply_package& ) > handler_func_t;

        public:
            request_receiver( const scarab::param_node& a_master_config );
            virtual ~request_receiver();

            void set_run_handler( const handler_func_t& a_func );
            void register_get_handler( const std::string& a_key, const handler_func_t& a_func );
            void register_set_handler( const std::string& a_key, const handler_func_t& a_func );
            void register_cmd_handler( const std::string& a_key, const handler_func_t& a_func );

            void remove_get_handler( const std::string& a_key );
            void remove_set_handler( const std::string& a_key );
            void remove_cmd_handler( const std::string& a_key );

            void execute();

        private:
            virtual bool do_run_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            virtual bool do_get_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            virtual bool do_set_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            virtual bool do_cmd_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            virtual void do_cancellation();

            handler_func_t f_run_handler;

            typedef std::unordered_map< std::string, handler_func_t > handler_funcs_t;
            handler_funcs_t f_get_handlers;
            handler_funcs_t f_set_handlers;
            handler_funcs_t f_cmd_handlers;

        public:
            enum status
            {
                k_initialized = 0,
                k_starting = 1,
                k_listening = 5,
                k_canceled = 9,
                k_done = 10,
                k_error = 100
            };

            static std::string interpret_status( status a_status );

            status get_status() const;
            void set_status( status a_status );

        private:
            std::atomic< status > f_status;

    };

    inline request_receiver::status request_receiver::get_status() const
    {
        return f_status.load();
    }

    inline void request_receiver::set_status( status a_status )
    {
        f_status.store( a_status );
        return;
    }

}

#endif
