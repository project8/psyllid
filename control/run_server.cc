/*
 * mt_run_server.cc
 *
 *  Created on: May 6, 2015
 *      Author: nsoblath
 */

#include "run_server.hh"

#include "daq_control.hh"
#include "daq_worker.hh"
#include "node_manager.hh"
#include "request_receiver.hh"
#include "signal_handler.hh"

#include "logger.hh"

#include <signal.h> // for raise()

using dripline::retcode_t;
using dripline::request_ptr_t;
using dripline::hub;

using scarab::param_node;
using scarab::version;

namespace psyllid
{
    LOGGER( plog, "run_server" );

    run_server::run_server( const scarab::param_node& a_node, const std::shared_ptr< scarab::version > a_version ) :
            f_config( a_node ),
            f_version( a_version ),
            f_return( RETURN_ERROR ),
            f_request_receiver(),
            f_daq_control(),
            f_node_manager(),
            f_component_mutex(),
            f_status( k_initialized )
    {
    }

    run_server::~run_server()
    {
    }

    void run_server::execute()
    {
        INFO( plog, "Creating server objects" );

        set_status( k_starting );

        signal_handler t_sig_hand;
        t_sig_hand.add_cancelable( this );

        // configuration manager
        //config_manager t_config_mgr( f_config, &t_dev_mgr );

        std::unique_lock< std::mutex > t_lock( f_component_mutex );

        // node manager
        f_node_manager.reset( new node_manager() );

        // daq control
        f_daq_control.reset( new daq_control( f_node_manager ) );

        // request receiver
        f_request_receiver.reset( new request_receiver( f_node_manager, f_daq_control ) );

        t_lock.unlock();

        INFO( plog, "Starting threads" );

        midge::thread t_receiver_thread;
        t_receiver_thread.start( f_request_receiver.get(), &request_receiver::execute );

        set_status( k_running );
        INFO( plog, "running..." );

        t_receiver_thread.join();

        t_sig_hand.remove_cancelable( this );

        set_status( k_done );

        t_lock.lock();
        f_request_receiver.reset();
        f_daq_control.reset();
        f_node_manager.reset();
        t_lock.unlock();

        INFO( plog, "Threads stopped" );

        f_return = RETURN_SUCCESS;

        return;
    }

    void run_server::do_cancellation()
    {
        std::unique_lock< std::mutex > t_lock( f_component_mutex );
        f_request_receiver.cancel();
        f_daq_control.cancel();
        f_node_manager.cancel();
        return;
    }

    void run_server::quit_server()
    {
        INFO( plog, "Shutting down the server" );
        cancel();
        return;
    }


    bool run_server::handle_get_server_status_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        param_node* t_server_node = new param_node();
        t_server_node->add( "status", new param_value( run_server::interpret_status( get_status() ) ) );

        f_component_mutex.lock();
        if( f_request_receiver != NULL )
        {
            param_node* t_rr_node = new param_node();
            t_rr_node->add( "status", new param_value( request_receiver::interpret_status( f_request_receiver->get_status() ) ) );
            t_server_node->add( "request-receiver", t_rr_node );
        }
        if( f_acq_request_db != NULL )
        {
            param_node* t_queue_node = new param_node();
            t_queue_node->add( "size", new param_value( (uint32_t)f_acq_request_db->queue_size() ) );
            t_queue_node->add( "is-active", new param_value( f_acq_request_db->queue_is_active() ) );
            t_server_node->add( "queue", t_queue_node );
        }
        if( f_server_worker != NULL )
        {
            param_node* t_sw_node = new param_node();
            t_sw_node->add( "status", new param_value( server_worker::interpret_status( f_server_worker->get_status() ) ) );
            t_sw_node->add( "digitizer-state", new param_value( server_worker::interpret_thread_state( f_server_worker->get_digitizer_state() ) ) );
            t_sw_node->add( "writer-state", new param_value( server_worker::interpret_thread_state( f_server_worker->get_writer_state() ) ) );
            t_server_node->add( "server-worker", t_sw_node );
        }
        f_component_mutex.unlock();

        a_reply_pkg.f_payload.add( "server", t_server_node );

        return a_reply_pkg.send_reply( retcode_t::success, "Server status request succeeded" );
    }

    bool run_server::handle_stop_all_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        param_node* t_server_node = new param_node();
        t_server_node->add( "status", new param_value( run_server::interpret_status( get_status() ) ) );

        f_component_mutex.lock();
        if( f_request_receiver != NULL )
        {
            param_node* t_rr_node = new param_node();
            t_rr_node->add( "status", new param_value( request_receiver::interpret_status( f_request_receiver->get_status() ) ) );
            t_server_node->add( "request-receiver", t_rr_node );
        }
        if( f_acq_request_db != NULL )
        {
            param_node* t_queue_node = new param_node();
            t_queue_node->add( "size", new param_value( (uint32_t)f_acq_request_db->queue_size() ) );
            t_queue_node->add( "is-active", new param_value( f_acq_request_db->queue_is_active() ) );
            t_server_node->add( "queue", t_queue_node );
        }
        if( f_server_worker != NULL )
        {
            param_node* t_sw_node = new param_node();
            t_sw_node->add( "status", new param_value( server_worker::interpret_status( f_server_worker->get_status() ) ) );
            t_sw_node->add( "digitizer-state", new param_value( server_worker::interpret_thread_state( f_server_worker->get_digitizer_state() ) ) );
            t_sw_node->add( "writer-state", new param_value( server_worker::interpret_thread_state( f_server_worker->get_writer_state() ) ) );
            t_server_node->add( "server-worker", t_sw_node );
        }
        f_component_mutex.unlock();

        a_reply_pkg.f_payload.add( "server", t_server_node );

        return a_reply_pkg.send_reply( retcode_t::success, "Server status request succeeded" );
    }

    bool run_server::handle_quit_server_request( const request_ptr_t, hub::reply_package& a_reply_pkg )
    {
        bool t_return = a_reply_pkg.send_reply( retcode_t::success, "Server-quit command processed" );
        quit_server();
        return t_return;
    }


    std::string run_server::interpret_status( status a_status )
    {
        switch( a_status )
        {
            case k_initialized:
                return std::string( "Initialized" );
                break;
            case k_starting:
                return std::string( "Starting" );
                break;
            case k_running:
                return std::string( "Running" );
                break;
            case k_done:
                return std::string( "Done" );
                break;
            case k_error:
                return std::string( "Error" );
                break;
            default:
                return std::string( "Unknown" );
        }
    }


} /* namespace mantis */
