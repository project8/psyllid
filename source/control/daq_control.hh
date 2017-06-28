/*
 * daq_control.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_DAQ_CONTROL_HH_
#define PSYLLID_DAQ_CONTROL_HH_

#include "control_access.hh"
#include "stream_manager.hh" // for midge_package
#include "psyllid_error.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include "hub.hh"
#include "relayer.hh"

#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

namespace psyllid
{
    class daq_control : public scarab::cancelable, public control_access
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
            daq_control( const scarab::param_node& a_master_config, std::shared_ptr< stream_manager > a_mgr );
            virtual ~daq_control();

            /// Pre-execution initialization (call after setting the control_access pointer)
            void initialize();

            /// Run the DAQ control thread
            void execute();

            /// Start the DAQ into the activated state
            /// Can throw daq_control::status_error; daq_control will still be usable
            /// Can throw psyllid::error; daq_control will NOT be usable
            void activate();
            /// Restarts the DAQ into the activated state
            /// Can throw daq_control::status_error; daq_control will still be usable
            /// Can throw psyllid::error; daq_control will NOT be usable
            void reactivate();
            /// Returns the DAQ to the deactivated state
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
            void apply_config( const std::string& a_node_name, const scarab::param_node& a_config );
            void dump_config( const std::string& a_node_name, scarab::param_node& a_config );

            /// Instruct a node to run a command
            /// Throws psyllid::error if the command fails; returns false if the command is not recognized
            bool run_command( const std::string& a_node_name, const std::string& a_cmd, const scarab::param_node& a_args );

        public:
            bool handle_activate_daq_control( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_reactivate_daq_control( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_deactivate_daq_control( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_start_run_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_stop_run_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_apply_config_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_dump_config_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_run_command_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_set_filename_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_set_description_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_set_duration_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

            bool handle_get_status_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_get_filename_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_get_description_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );
            bool handle_get_duration_request( const dripline::request_ptr_t a_request, dripline::reply_package& a_reply_pkg );

        private:
            void do_cancellation();

            void do_run( unsigned a_duration );

            std::condition_variable f_activation_condition;
            std::mutex f_daq_mutex;

            std::shared_ptr< stream_manager > f_node_manager;

            std::unique_ptr< scarab::param_node > f_daq_config;

            midge_package f_midge_pkg;
            active_node_bindings* f_node_bindings;

            std::condition_variable f_run_stopper; // ends the run after a given amount of time
            std::mutex f_run_stop_mutex; // mutex used by the run_stopper
            bool f_do_break_run; // bool to confirm that the run should stop; protected by f_run_stop_mutex

            std::future< void > f_run_return;

            dripline::relayer f_dl_relay;
            std::string f_slack_queue;

        public:
            void set_filename( const std::string& a_filename, unsigned a_file_num = 0 );
            const std::string& get_filename( unsigned a_file_num = 0 );

            void set_description( const std::string& a_desc, unsigned a_file_num = 0 );
            const std::string& get_description( unsigned a_file_num = 0 );

            mv_accessible( unsigned, run_duration );

        public:
            enum class status:uint32_t
            {
                deactivated = 0,
                activating = 2,
                activated = 4,
                running = 5,
                deactivating = 6,
                canceled = 8,
                do_restart = 9,
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
