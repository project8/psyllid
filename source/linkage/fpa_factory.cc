/*
 * fpa_factory.cc
 *
 *  Created on: Sep 23, 2016
 *      Author: nsoblath
 */

#include "fpa_factory.hh"

namespace psyllid
{

    fpa_ptr fpa_factory::get_fpa( const std::string& a_interface )
    {
        if( a_interface.empty() ) return fpa_ptr();
        fpa_map_it t_it = f_fpa_map.find( a_interface );
        if( t_it == f_fpa_map.end() )
        {
            fpa_ptr t_fpa = std::make_shared< fast_packet_acq >( a_interface );
            f_fpa_map[ a_interface ] = t_fpa;
            return t_fpa;
        }
        return t_it->second;
    }

    fpa_factory::fpa_factory() :
            scarab::singleton< fpa_factory >(),
            f_fpa_map()
    {
    }

    fpa_factory::~fpa_factory()
    {
        // fpa's are not owned by the factory, and are therefore not deleted here.
        // memory management is taken care of by using shared pointers.
    }

} /* namespace psyllid */
