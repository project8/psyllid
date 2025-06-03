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
            f_data_format( 
                { 
                    4096, 
                    2, 
                    1, 
                    0, 
                    0 
                } 
                ),
            f_has_data( false )
    {
    }

    generic_time_data::generic_time_data(
        uint32_t record_size,
        uint32_t sample_number,
        size_t data_type_size,
        uint32_t data_type_format,
        uint32_t data_type_sign
    ) : 
            f_data_format( 
                { 
                    record_size, 
                    sample_number, 
                    data_type_size, 
                    data_type_format, 
                    data_type_sign 
                } 
                )
    {

    }



    generic_time_data::~generic_time_data()
    {
        delete [] f_byte_array;
    }

    // void generic_time_data::initialize()
    // {

    // }

} /* namespace psyllid */
