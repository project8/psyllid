#define MANTIS_API_EXPORTS

#include "parser.hh"

#include "parsable.hh"

#include <sstream>


namespace psyllid
{
    using scarab::param_value;
    using scarab::parsable;

    parser::parser( int an_argc, char** an_argv ) :
            param_node()
    {
        parse(an_argc, an_argv);
    }

    parser::~parser()
    {
    }

    void parser::parse( int an_argc, char** an_argv )
    {
        if( an_argc > 0)
        {
            this->replace( "executable", param_value( an_argv[ 0 ] ) );
        }

        for( int t_index = 1; t_index < an_argc; t_index++ )
        {
            //t_argument.assign( an_argv[ t_index ] );
            parsable t_arg( an_argv[ t_index ] );
            merge( t_arg );
        }

        return;
    }


}
