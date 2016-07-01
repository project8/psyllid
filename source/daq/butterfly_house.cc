/*
 * butterfly_house.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 */

#include "butterfly_house.hh"

#include "logger.hh"

#include "psyllid_error.hh"

namespace psyllid
{
    LOGGER( plog, "butterfly_house" );


    butterfly_house::butterfly_house() :
            f_butterflies(),
            f_house_mutex()
    {
    }

    butterfly_house::~butterfly_house()
    {
    }

    monarch_wrap_ptr butterfly_house::declare_file( const std::string& a_filename )
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
        try
        {
            auto t_mwp_it = f_butterflies.find( a_filename );
            if( t_mwp_it == f_butterflies.end() )
            {
                monarch_wrap_ptr t_mwp( new monarch_wrapper( a_filename ) );
                LINFO( plog, "Created egg3 file <" << a_filename << ">" );
                f_butterflies.insert( bf_value_t( a_filename, t_mwp ) );
                return t_mwp;
            }
            return t_mwp_it->second;
        }
        catch( error& e )
        {
            throw;
        }
    }
/*
    header_wrap_ptr butterfly_house::get_header( const std::string& a_filename ) const
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
        try
        {
            bf_cit_t t_it = f_butterflies.find( a_filename );
            if( t_it == f_butterflies.end() )
            {
                throw error() << "No Monarch present with filename <" << a_filename << ">";
            }
            return t_it->second->get_header();
        }
        catch( error& e )
        {
            throw;
        }
    }

    stream_wrap_ptr butterfly_house::get_stream( const std::string& a_filename, unsigned a_stream_no ) const
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
        try
        {
            bf_cit_t t_it = f_butterflies.find( a_filename );
            if( t_it == f_butterflies.end() )
            {
                throw error() << "No Monarch present with filename <" << a_filename << ">";
            }
            return t_it->second->get_stream( a_stream_no );
        }
        catch( error& e )
        {
            throw;
        }
    }

    void butterfly_house::write_file( const std::string& a_filename )
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
        try
        {
            bf_it_t t_it = f_butterflies.find( a_filename );
            if( t_it == f_butterflies.end() )
            {
                throw error() << "No Monarch present with filename <" << a_filename << ">";
            }
            t_it->second->finish_file();
            return;
        }
        catch( error& e )
        {
            throw;
        }
    }
*/

} /* namespace psyllid */
