/*
 * generic_time_data.cc
 *
 *  Created on: May 30, 2025
 *      Author: pkolbeck
 */

#include "generic_time_data.hh"

namespace psyllid
{

    generic_time_data::generic_time_data() :
            f_egg_header(),
            f_data_format( 
                { 
                    4096, 
                    2, 
                    DT_UINT8
                } 
                ),
            f_has_data( false )
    {
    }

    generic_time_data::generic_time_data(
        size_t record_size,
        uint32_t sample_number,
        generic_time_data::DataType data_type
    ) : 
            f_egg_header(),
            f_data_format( 
                { 
                    record_size, 
                    sample_number, 
                    data_type 
                } 
                ),
            f_has_data( false )
    {
    }

    generic_time_data::generic_time_data(
        monarch3::M3Header egg_header
    ) : 
            f_egg_header( egg_header ),
            f_data_format( 
                { 
                    4096, 
                    2, 
                    DT_UINT8
                } 
                ),
            f_has_data( false )
    {
    }

    generic_time_data::generic_time_data(
        size_t record_size,
        uint32_t sample_number,
        generic_time_data::DataType data_type,
        monarch3::M3Header egg_header
    ) : 
            f_egg_header( egg_header ),
            f_data_format( 
                { 
                    record_size, 
                    sample_number, 
                    DT_UINT8
                } 
                ),
            f_has_data( false )
    {
    }

    generic_time_data::~generic_time_data()
    {
        delete [] f_byte_array;
    }

    generic_time_data& generic_time_data::operator=( const generic_time_data& a_orig )
    {
        // malloc can be expensive. 
        return *this;
    }


} /* namespace psyllid */
