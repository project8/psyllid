/*
 * daq_control.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_DAQ_CONTROL_HH_
#define PSYLLID_DAQ_CONTROL_HH_

#include "member_variables.hh"

#include "psyllid_error.hh"

#include "cancelable.hh"

#include "hub.hh"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

namespace psyllid
{
    class daq_worker;
    class node_manager;

    class daq_control : public midge::cancelable
    {
        public:
            class run_error : public error
            {
                public:
                    run_error() {}
                    virtual ~run_error() {}
            };
            class status_error : public error
            {
                public:
                    status_error() {}
                    virtual ~status_error() {}
            };

        public:
            daq_control( const scarab::param_node& a_master_config, std::shared_ptr< node_manager > a_mgr );
            virtual ~daq_control();

            /// Run the DAQ control thread
            void execute( std::exception_ptr a_ex_ptr );

            /// Start the DAQ into the idle state
            /// Deactivated with deactivate()
            /// Can throw daq_control::status_error; daq_control will still be usable
            /// Can throw psyllid::error; daq_control will NOT be usable
            void activate();
            /// Returns the DAQ to the initialized state
            /// Can throw daq_control::status_error; daq_control will still be usable
            /// Can throw psyllid::error; daq_control will NOT be usable
            void deactivate();

            /// Start a run
            /// Can throw daq_control::run_error or status_error; daq_control will still be usable
            /// Can throw psyllid::error; daq_control will NOT be usable
            /// Stop run with stop_run()
            void start_run();

            /// Stop a run
            /// Can throw daq_control::run_error or status_error; daq_control will still be usable
            /// Can throw psyllid::error; daq_control will NOT be usable
            void stop_run();

        public:
            bool handle_activate_daq_control( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_deactivate_daq_control( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_start_run_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_stop_run_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_set_filename_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_set_description_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_set_duration_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_get_status_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

        private:
            void notify_run_stopped();

            void do_cancellation();

            std::condition_variable f_condition;
            std::mutex f_daq_mutex;

            std::shared_ptr< node_manager > f_node_manager;

            std::shared_ptr< daq_worker > f_daq_worker;
            std::mutex f_worker_mutex;
            std::thread f_worker_thread;

            std::unique_ptr< scarab::param_node > f_daq_config;

        public:
            mv_referrable( std::string, run_filename );
            mv_referrable( std::string, run_description );
            mv_accessible( unsigned, run_duration );

        public:
            enum class status:uint32_t
            {
                initialized = 0,
                activating = 2,
                idle = 4,
                running = 5,
                deactivating = 6,
                canceled = 8,
                done = 10,
                error = 200
            };

            static uint32_t status_to_uint( status a_status );
            static status uint_to_status( uint32_t a_value );
            static std::string interpret_status( status a_status );

            status get_status() const;
            void set_status( status a_status );

        private:
            std::atomic< status > f_status;


    };

    inline daq_control::status daq_control::get_status() const
    {
        return f_status.load();
    }

    inline void daq_control::set_status( status a_status )
    {
        f_status.store( a_status );
        return;
    }


} /* namespace psyllid */

#endif /* PSYLLID_DAQ_CONTROL_HH_ */
