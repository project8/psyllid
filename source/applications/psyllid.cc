/*
 * psyllid.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: N.S. Oblath
 */

#include "daq_control.hh"
#include "psyllid_error.hh"
#include "psyllid_version.hh"

#include "conductor.hh"
#include "sandfly_error.hh"
#include "server_config.hh"

#include "application.hh"
#include "logger.hh"

using namespace psyllid;
using namespace sandfly;

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
        conductor the_conductor;
        the_conductor.set_rc_creator< daq_control >();

        // Default configuration
        the_main.default_config() = server_config();

        // The main execution callback
        the_main.callback( [&](){ the_conductor.execute( the_main.primary_config() ); } );

        // Command line options
        add_sandfly_options( the_main );

        // Package version
        the_main.set_version( std::make_shared< psyllid::version >() );

        // Parse CL options and run the application
        CLI11_PARSE( the_main, argc, argv );

        return the_conductor.get_return();

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
    catch( sandfly::error& e )
    {
        LERROR( plog, "sandfly error: " << e.what() );
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
