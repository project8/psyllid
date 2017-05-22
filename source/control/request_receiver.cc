
#include "request_receiver.hh"

#include "psyllid_error.hh"

#include "logger.hh"
#include "parsable.hh"

#include <cstddef>
#include <signal.h>
#include <sstream>

using std::string;

using scarab::parsable;
using scarab::param_node;

using dripline::retcode_t;
using dripline::request_ptr_t;


namespace psyllid
{

    LOGGER( plog, "request_receiver" );

    request_receiver::request_receiver( const param_node& a_master_config ) :
            hub(),
            scarab::cancelable(),
            f_run_handler(),
            f_get_handlers(),
            f_set_handlers(),
            f_cmd_handlers(),
            f_listen_timeout_ms( 100 ),
            f_amqp_config(),
            f_status( k_initialized )
    {
        if( ! a_master_config.has( "amqp" ) )
        {
            throw error() << "No AMQP configuration present";
        }
        f_amqp_config.reset( new param_node( a_master_config.node_at( "amqp" ) ) );
    }

    request_receiver::~request_receiver()
    {
    }

    void request_receiver::set_run_handler( const handler_func_t& a_func )
    {
        f_run_handler = a_func;
        LDEBUG( plog, "Set RUN handler" );
        return;
    }

    void request_receiver::register_get_handler( const std::string& a_key, const handler_func_t& a_func )
    {
        f_get_handlers[ a_key ] = a_func;
        LDEBUG( plog, "Set GET handler for <" << a_key << ">" );
        return;
    }

    void request_receiver::register_set_handler( const std::string& a_key, const handler_func_t& a_func )
    {
        f_set_handlers[ a_key ] = a_func;
        LDEBUG( plog, "Set SET handler for <" << a_key << ">" );
        return;
    }

    void request_receiver::register_cmd_handler( const std::string& a_key, const handler_func_t& a_func )
    {
        f_cmd_handlers[ a_key ] = a_func;
        LDEBUG( plog, "Set CMD handler for <" << a_key << ">" );
        return;
    }

    void request_receiver::remove_get_handler( const std::string& a_key )
    {
        if( f_get_handlers.erase( a_key ) == 0 )
        {
            LWARN( plog, "GET handler <" << a_key << "> was not present; nothing was removed" );
        }
        else
        {
            LDEBUG( plog, "GET handler <" << a_key << "> was removed" );
        }
        return;
    }

    void request_receiver::remove_set_handler( const std::string& a_key )
    {
        if( f_set_handlers.erase( a_key ) == 0 )
        {
            LWARN( plog, "SET handler <" << a_key << "> was not present; nothing was removed" );
        }
        else
        {
            LDEBUG( plog, "SET handler <" << a_key << "> was removed" );
        }
        return;
    }

    void request_receiver::remove_cmd_handler( const std::string& a_key )
    {
        if( f_cmd_handlers.erase( a_key ) == 0 )
        {
            LWARN( plog, "CMD handler <" << a_key << "> was not present; nothing was removed" );
        }
        else
        {
            LDEBUG( plog, "CMD handler <" << a_key << "> was removed" );
        }
        return;
    }

    void request_receiver::execute()
    {
        set_status( k_starting );

        try
        {
            if( ! dripline_setup( f_amqp_config->get_value( "broker" ),
                                  f_amqp_config->get_value< unsigned >( "broker-port" ),
                                  f_amqp_config->get_value( "request-exchange" ),
                                  f_amqp_config->get_value( "queue" ),
                                  ".project8_authentications.json" ) )
            {
                LERROR( plog, "Unable to complete dripline setup" );
                raise( SIGINT );
                return;
            }
        }
        catch( scarab::error& e )
        {
            LERROR( plog, "Invalid AMQP configuration" );
            raise( SIGINT );
            return;
        }

        f_listen_timeout_ms = f_amqp_config->get_value< int >( "listen-timeout-ms", f_listen_timeout_ms );

        // start the service
        if( ! start() )
        {
            LERROR( plog, "Unable to start the dripline service" );
            raise( SIGINT );
            return;
        }

        LINFO( plog, "Waiting for incoming messages" );

        set_status( k_listening );

        while( ! cancelable::is_canceled() )
        {
            // blocking call to wait for incoming message
            listen( f_listen_timeout_ms );
        }

        LINFO( plog, "No longer waiting for messages" );

        stop();

        set_status( k_done );
        LDEBUG( plog, "Request receiver is done" );

        return;
    }

    bool request_receiver::do_run_request( const request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        return f_run_handler( a_request, a_reply_pkg );
    }

    bool request_receiver::do_get_request( const request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        std::string t_query_type = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        try
        {
            return f_get_handlers.at( t_query_type )( a_request, a_reply_pkg );
        }
        catch( std::out_of_range& e )
        {
            LWARN( plog, "GET query type <" << t_query_type << "> was not understood (" << e.what() << ")" );
            return a_reply_pkg.send_reply( retcode_t::message_error_bad_payload, "Unrecognized query type or no query type provided: <" + t_query_type + ">" );;
        }
    }

    bool request_receiver::do_set_request( const request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        std::string t_set_type = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        try
        {
            return f_set_handlers.at( t_set_type )( a_request, a_reply_pkg );
        }
        catch( std::out_of_range& e )
        {
            LWARN( plog, "SET request <" << t_set_type << "> not understood (" << e.what() << ")" );
            return a_reply_pkg.send_reply( retcode_t::message_error_bad_payload, "Unrecognized set request type or no set request type provided: <" + t_set_type + ">" );
        }
    }

    bool request_receiver::do_cmd_request( const request_ptr_t a_request, dripline::reply_package& a_reply_pkg )
    {
        // get the instruction before checking the lockout key authentication because we need to have the exception for
        // the unlock instruction that allows us to force the unlock.
        std::string t_instruction = a_request->parsed_rks().front();
        a_request->parsed_rks().pop_front();

        try
        {
            return f_cmd_handlers.at( t_instruction )( a_request, a_reply_pkg );
        }
        catch( std::out_of_range& e )
        {
            LWARN( plog, "CMD instruction <" << t_instruction << "> not understood (" << e.what() << ")" );
            return a_reply_pkg.send_reply( retcode_t::message_error_bad_payload, "Instruction <" + t_instruction + "> not understood" );;
        }
    }

    void request_receiver::do_cancellation()
    {
        LDEBUG( plog, "Canceling request receiver" );
        service::f_canceled.store( true );
        set_status( k_canceled );
        return;
    }

    std::string request_receiver::interpret_status( status a_status )
    {
        switch( a_status )
        {
            case k_initialized:
                return std::string( "Initialized" );
                break;
            case k_starting:
                return std::string( "Starting" );
                break;
            case k_listening:
                return std::string( "Listening" );
                break;
            case k_canceled:
                return std::string( "Canceled" );
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


}
