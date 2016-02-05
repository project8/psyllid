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
#include "thread.hh"

#include "hub.hh"

#include <atomic>
#include <condition_variable>
#include <memory>

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
            daq_control( std::shared_ptr< node_manager > a_mgr );
            virtual ~daq_control();

            /// Run the DAQ control thread
            void execute();

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
            bool handle_activate_daq_control( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg );

            bool handle_deactivate_daq_control( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg );

            bool handle_start_run_request( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg );

            bool handle_stop_run_request( const dripline::request_ptr_t a_request, dripline::hub::reply_package& a_reply_pkg );

        private:
            void notify_run_stopped();

            void do_cancellation();

            std::condition_variable f_condition;
            std::mutex f_daq_mutex;

            std::shared_ptr< node_manager > f_node_manager;

            std::shared_ptr< daq_worker > f_daq_worker;
            std::mutex f_worker_mutex;
            midge::thread f_worker_thread;

        public:
            enum class status
            {
                initialized,
                activating,
                idle,
                running,
                deactivating,
                canceled,
                done,
                error
            };

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
