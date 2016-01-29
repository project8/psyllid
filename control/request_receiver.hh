#ifndef PSYLLID_REQUEST_RECEIVER_HH_
#define PSYLLID_REQUEST_RECEIVER_HH_

#include "hub.hh"

#include "param.hh"

#include "cancelable.hh"

#include <atomic>
#include <memory>

namespace psyllid
{

    class node_manager;
    class daq_control;
    class condition;

    class request_receiver : public dripline::hub, public midge::cancelable
    {
        public:
            request_receiver( std::shared_ptr< node_manager > a_node_manager, std::shared_ptr< daq_control > a_daq_control );
            virtual ~request_receiver();

            void execute();
            void cancel();

        private:
            virtual bool do_run_request( const dripline::request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual bool do_get_request( const dripline::request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual bool do_set_request( const dripline::request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual bool do_cmd_request( const dripline::request_ptr_t a_request, reply_package& a_reply_pkg );

            int f_listen_timeout_ms;

            std::shared_ptr< node_manager > f_node_manager;
            std::shared_ptr< daq_control > f_daq_control;

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
