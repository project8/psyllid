/*
 * iq_time_data.cc
 *
 *  Created on: Mar 21, 2025
 *      Author: pkolbeck
 */

#include "iq_time_data.hh"

#include <cstring>

namespace psyllid
{

    iq_time_data::iq_time_data() :
            f_pkt_in_session( 0 ),
            f_owns_data( false ),
            f_array( nullptr ),
            f_array_size( 0 )
    {}

    void iq_time_data::initialize( size_t a_size, iq_time_data::iq_t* a_external_array = nullptr )
    {
        // first take care of existing array, if there is any
        if( f_owns_data )
        {
            delete [] f_array;
        }

        // then setup the new array
        f_array_size = a_size;
        f_owns_data = a_external_array == nullptr;
        if( f_owns_data )
        {
            f_array = new iq_t[f_array_size];
        }
        else
        {
            f_array = a_external_array;
        }

        return;
    }

    iq_time_data& iq_time_data::operator=( const iq_time_data& a_orig )
    {
        if( f_owns_data && a_orig.f_owns_data && f_array_size != a_orig.f_array_size )
        {
            // shortcut case: both objects own the data and the array size matches
            std::memcpy( f_array, a_orig.f_array, f_array_size * sizeof(iq_t) );
        }
        else
        {
            // generic case: dynamically delete and allocate arrays as needed based on object ownership
            // if a_orig doesn't own the data, then just copy over the pointer
            if( f_owns_data )
            {
                delete [] f_array;
            }
            if( a_orig.f_owns_data )
            {
                f_array = new iq_t[a_orig.f_array_size];
                std::memcpy( f_array, a_orig.f_array, a_orig.f_array_size * sizeof(iq_t) );
            }
            else
            {
                f_array = a_orig.f_array;
            }
        }

        f_pkt_in_session = a_orig.f_pkt_in_session;
        f_owns_data = a_orig.f_owns_data;
        f_array_size = a_orig.f_array_size;

        return *this;
    }

    iq_time_data::~iq_time_data()
    {
        if( f_owns_data )
        {
            delete [] f_array;
        }
    }

} /* namespace psyllid */
