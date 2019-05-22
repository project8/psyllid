/*
 * roach_daq_streaming_1chan.cc
 *
 *  Created on: Dec 5, 2016
 *      Author: nsoblath
 *
 *  Streaming DAQ for the ROACH
 *
 *  Usage: > roach_daq_streaming_1chan [options]
 *
 *  Parameters:
 *    - port: (uint) port number to listen on for packets
 *    - interface: (string) network interface name to listen on for packets; this is only needed if using the FPA receiver; default is "eth1"
 *    - ip: (string) IP address to listen on for packets; this is only needed if using the socket receiver; default is "127.0.0.1"
 *    - fpa: (null) Flag to request use of the FPA receiver; only valid on linux machines
 */


#include "psyllid_error.hh"
#include "psyllid_version.hh"
#include "packet_receiver_socket.hh"
#include "streaming_writer.hh"
#include "terminator.hh"
#include "tf_roach_receiver.hh"

#ifdef __linux__
#include "packet_receiver_fpa.hh"
#endif

#include "diptera.hh"

#include "application.hh"
#include "logger.hh"
#include "param.hh"

#include "dripline_constants.hh" // for RETURN constants

#include <signal.h>

using namespace psyllid;

LOGGER( plog, "test_tf_roach_receiver" );

scarab::cancelable* f_cancelable = nullptr;

void cancel( int )
{
    LINFO( plog, "Attempting to cancel" );
    if( f_cancelable != nullptr ) f_cancelable->cancel();
    return;
}

int main( int argc, char** argv )
{
    try
    {
        // The application
        scarab::main_app the_main;

        // Default configuration
        scarab::param_node t_default_config;
        t_default_config.add( "ip", scarab::param_value( "127.0.0.1" ) );
        t_default_config.add( "port", scarab::param_value( 23530 ) );
        t_default_config.add( "interface", scarab::param_value( "eth1" ) );
        the_main.default_config() = t_default_config;

        // Command line options
        the_main.add_config_option< std::string >( "--ip", "ip", "IP address from which to receive packets" );
        the_main.add_config_option< unsigned >( "-p,--port", "port", "Port on which to receive packets" );
        the_main.add_config_option< std::string >( "-i,--interface", "interface", "Ethernet interface to grab packets off of" );
        the_main.add_config_flag< bool >( "-f,--fpa", "fpa", "Enable use of the FPA" );

        // Package version
        the_main.set_version( new psyllid::version() );

        // The main execution callback
        the_main.callback( [&]() {
            std::string t_ip( the_main.master_config()["ip"]().as_string() );
            unsigned t_port = the_main.master_config()["port"]().as_uint();
            std::string t_interface( the_main.master_config()["interface"]().as_string() );
            bool t_use_fpa( the_main.master_config().has( "fpa" ) );

            LINFO( plog, "Creating and configuring nodes" );

            midge::diptera* t_root = new midge::diptera();

            if( t_use_fpa )
            {
    #ifdef __linux__
                packet_receiver_fpa* t_pck_rec = new packet_receiver_fpa();
                t_pck_rec->set_name( "pck_rec" );
                t_pck_rec->set_length( 10 );
                t_pck_rec->set_port( t_port );
                t_pck_rec->interface() = t_interface;
                t_root->add( t_pck_rec );
                f_cancelable = t_pck_rec;
    #else
                throw error() << "FPA was requested, but is only available on a Linux machine";
    #endif
            }
            else
            {
                packet_receiver_socket* t_pck_rec = new packet_receiver_socket();
                t_pck_rec->set_name( "pck_rec" );
                t_pck_rec->set_length( 10 );
                t_pck_rec->set_port( t_port );
                t_pck_rec->ip() = t_ip;
                t_root->add( t_pck_rec );
                f_cancelable = t_pck_rec;
            }

            tf_roach_receiver* t_tfr_rec = new tf_roach_receiver();
            t_tfr_rec->set_name( "tfr_rec" );
            t_tfr_rec->set_time_length( 10 );
            t_tfr_rec->set_start_paused( false );
            t_root->add( t_tfr_rec );

            streaming_writer* t_str_wrt = new streaming_writer();
            t_str_wrt->set_name( "strw" );
            t_root->add( t_str_wrt );

            terminator_freq_data* t_term_f = new terminator_freq_data();
            t_term_f->set_name( "term_f" );
            t_root->add( t_term_f );

            LINFO( plog, "Connecting nodes" );

            t_root->join( "pck_rec.out_0:tfr_rec.in_0" );
            t_root->join( "tfr_rec.out_0:strw.in_0" );
            t_root->join( "tfr_rec.out_1:term_f.in_0" );

            LINFO( plog, "Exit with ctrl-c" );

            // set up signal handling for canceling with ctrl-c
            signal( SIGINT, cancel );

            LINFO( plog, "Executing" );

            std::exception_ptr t_e_ptr = t_root->run( "pck_rec:tfr_rec:strw:term_f" );

            if( t_e_ptr ) std::rethrow_exception( t_e_ptr );

            LINFO( plog, "Execution complete" );

            // un-setup signal handling
            f_cancelable = nullptr;

            delete t_root;
        } );

        // Parse CL options and run the application
        CLI11_PARSE( the_main, argc, argv );

        return RETURN_SUCCESS;
    }
    catch( std::exception& e )
    {
        LERROR( plog, "Exception caught: " << e.what() );
    }
    return RETURN_ERROR;

}



