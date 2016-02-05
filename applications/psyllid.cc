/*
 * psyllid.cc
 *
 *  Created on: Feb 1, 2016
 *      Author: nsoblath
 */

#include "configurator.hh"
#include "psyllid_constants.hh"
#include "psyllid_error.hh"
#include "psyllid_version.hh"
#include "run_server.hh"
#include "server_config.hh"

#include "logger.hh"

using namespace psyllid;

using std::string;

LOGGER( plog, "psyllid" );

set_version( psyllid, version );

int main( int argc, char** argv )
{
    INFO( plog, "Welcome to Psyllid\n\n" <<
            "\t\t                                    _/  _/  _/        _/   \n" <<
            "\t\t     _/_/_/      _/_/_/  _/    _/  _/  _/        _/_/_/    \n" <<
            "\t\t    _/    _/  _/_/      _/    _/  _/  _/  _/  _/    _/     \n" <<
            "\t\t   _/    _/      _/_/  _/    _/  _/  _/  _/  _/    _/      \n" <<
            "\t\t  _/_/_/    _/_/_/      _/_/_/  _/  _/  _/    _/_/_/       \n" <<
            "\t\t _/                        _/                              \n" <<
            "\t\t_/                    _/_/                                 \n\n");

    try
    {
        server_config t_sc;
        configurator t_configurator( argc, argv, &t_sc );

        // Run the server
        run_server the_server( t_configurator.config(), std::shared_ptr< scarab::version_semantic >( new version() ) );

        the_server.execute();

        return the_server.get_return();
    }
    catch( scarab::error& e )
    {
        ERROR( plog, "configuration error: " << e.what() );
        return RETURN_ERROR;
    }
    catch( psyllid::error& e )
    {
        ERROR( plog, "psyllid error: " << e.what() );
        return RETURN_ERROR;
    }
    catch( std::exception& e )
    {
        ERROR( plog, "std::exception caught: " << e.what() );
        return RETURN_ERROR;
    }
    catch( ... )
    {
        ERROR( plog, "unknown excpetion caught" );
        return RETURN_ERROR;
    }

    return RETURN_ERROR;
}




