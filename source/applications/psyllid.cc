/*
 * psyllid.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: nsoblath
 */

#include "psyllid_constants.hh"
#include "psyllid_error.hh"
#include "psyllid_version.hh"
#include "run_server.hh"
#include "server_config.hh"

#include "application.hh"
#include "logger.hh"

using namespace psyllid;

using std::string;

LOGGER( plog, "psyllid" );

int main( int argc, char** argv )
{
    LINFO( plog, "Welcome to Psyllid\n\n" <<
            "\t\t                                    _/  _/  _/        _/   \n" <<
            "\t\t     _/_/_/      _/_/_/  _/    _/  _/  _/        _/_/_/    \n" <<
            "\t\t    _/    _/  _/_/      _/    _/  _/  _/  _/  _/    _/     \n" <<
            "\t\t   _/    _/      _/_/  _/    _/  _/  _/  _/  _/    _/      \n" <<
            "\t\t  _/_/_/    _/_/_/      _/_/_/  _/  _/  _/    _/_/_/       \n" <<
            "\t\t _/                        _/                              \n" <<
            "\t\t_/                    _/_/                                 \n\n");

    try
    {
        // The application
        scarab::main_app the_main;
        run_server the_server;

        // Default configuration
        the_main.default_config() = server_config();

        // The main execution callback
        the_main.callback( [&](){ the_server.execute( the_main.master_config() ); } );

        // Command line options
        the_main.add_config_option< std::string >( "-b,--broker", "amqp.broker", "Set the dripline broker address" );
        the_main.add_config_option< unsigned >( "-p,--port", "amqp.broker-port", "Set the port for communication with the dripline broker" );
        the_main.add_config_option< std::string >( "-e,--exchange", "amqp.exchange", "Set the exchange to send message on" );
        the_main.add_config_option< std::string >( "-a,--auth-file", "amqp.auth-file", "Set the authentication file path" );
        the_main.add_config_option< std::string >( "-s,--slack-queue", "amqp.slack-queue", "Set the queue name for Slack messages" );
        the_main.add_config_option< bool >( "--post-to-slack", "post-to-slack", "Flag for en/disabling posting to Slack" );
        the_main.add_config_option< bool >( "--activate-at-startup", "daq.activate-at-startup", "Flag to make Psyllid activate on startup" );
        the_main.add_config_option< unsigned >( "-n,--n-files", "daq.n-files", "Number of files to be written in parallel" );
        the_main.add_config_option< unsigned >( "-d,--duration", "daq.duration", "Run duration in ms" );
        the_main.add_config_option< double >( "-m,--max-file-size-mb", "daq.max-file-size-mb", "Maximum file size in MB" );

        // Package version
        the_main.set_version( new psyllid::version() );

        // Parse CL options and run the application
        CLI11_PARSE( the_main, argc, argv );

        return the_server.get_return();
    }
    catch( scarab::error& e )
    {
        LERROR( plog, "configuration error: " << e.what() );
        return RETURN_ERROR;
    }
    catch( psyllid::error& e )
    {
        LERROR( plog, "psyllid error: " << e.what() );
        return RETURN_ERROR;
    }
    catch( std::exception& e )
    {
        LERROR( plog, "std::exception caught: " << e.what() );
        return RETURN_ERROR;
    }
    catch( ... )
    {
        LERROR( plog, "unknown exception caught" );
        return RETURN_ERROR;
    }

    return RETURN_ERROR;
}




