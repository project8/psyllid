#include <iostream>

#include "psyllid_error.hh"

namespace psyllid
{

    error::error() :
            exception(),
            f_message( "" )
    {
    }
    error::error( const error& p_copy ) :
            exception( p_copy ),
            f_message( p_copy.f_message )
    {
    }
    error& error::operator=( const error& p_copy )
    {
        exception::operator=( p_copy );
        f_message = p_copy.f_message;
        return *this;
    }
    error::~error() throw()
    {
    }

    const char* error::what() const throw()
    {
        return f_message.c_str();
    }

    fatal_error::fatal_error() :
            exception(),
            f_message( "" )
    {
    }
    fatal_error::fatal_error( const fatal_error& p_copy ) :
            exception( p_copy ),
            f_message( p_copy.f_message )
    {
    }
    fatal_error& fatal_error::operator=( const fatal_error& p_copy )
    {
        exception::operator=( p_copy );
        f_message = p_copy.f_message;
        return *this;
    }
    fatal_error::~fatal_error() throw()
    {
    }

    const char* fatal_error::what() const throw()
    {
        return f_message.c_str();
    }

}

