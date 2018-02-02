
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
            hub( a_master_config.node_at( "amqp" ) ),
            scarab::cancelable(),
            f_status( k_initialized )
    {
    }

    request_receiver::~request_receiver()
    {
    }

    void request_receiver::execute()
    {
        set_status( k_starting );

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
            listen();
        }

        LINFO( plog, "No longer waiting for messages" );

        stop();

        set_status( k_done );
        LDEBUG( plog, "Request receiver is done" );

        return;
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
