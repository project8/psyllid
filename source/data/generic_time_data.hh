/*
 * generic_time_data.hh
 *
 *  Created on: May 30, 2025
 *      Author: pkolbeck
 */

#ifndef PSYLLID_GENERIC_TIME_DATA_HH_
#define PSYLLID_GENERIC_TIME_DATA_HH_

#include "member_variables.hh"
#include "M3Header.hh"

#include <cinttypes>
#include <cstddef>
#include <string>

namespace psyllid
{

    // object storing characteristics of the data in generic_time_data
    struct current_data_format
    {
            uint32_t record_size;
            uint32_t sample_number;
            uint32_t data_type_size;
            uint32_t data_format_type;
            uint32_t data_type_sign;
    };

    class generic_time_data
    {
        public:
            // We start by wanting the compiler to catch any and all possible problems. If the program compiles, it should run, up to numeric factors. 
            // We want type specific constructors, where the template type is that of the stored data. 
            // The egg reader should then, having determined what the type of the data in the egg file is, produce a generic_time_data with that type in it. 
            // Does it make the most sense to propagate the egg header? 
            // I think so, that should have all information in it regarding how the simulation was made. 
            // If its not in there, then the 'converter node' should have some parameter that uses that information. 
            generic_time_data();
            generic_time_data(
                uint32_t record_size,
                uint32_t sample_number,
                uint32_t data_type_size,
                uint32_t data_format_type,
                uint32_t data_type_sign
            );
            virtual ~generic_time_data();

            void initialize();

        public:
            typedef uint8_t byte_type;

            // functions to return the byte array
            const byte_type* get_byte_array() const;
            byte_type* get_byte_array();
            size_t get_byte_array_size() const;

            // functions to return the byte array, reinterpret cast
            
            // functions to return the data entries themselves, static cast

            // returning data characteristics
            mv_accessible( uint64_t, pkt_in_session );




        private:
            byte_type* f_byte_array;
            size_t f_byte_array_size;
            monarch3::M3Header f_egg_header;
            const current_data_format f_data_format;

    };

    inline const generic_time_data::byte_type* generic_time_data::get_byte_array() const
    {
        return f_byte_array;
    }

    inline generic_time_data::byte_type* generic_time_data::get_byte_array()
    {
        return f_byte_array;
    }

    inline size_t generic_time_data::get_byte_array_size() const
    {
        return f_byte_array_size;
    }

    // template< typename type_t >
    // const generic_time_data::type_t* generic_time_data::get_array() const
    // {
    //     return reinterpret_cast< type_t* >( f_byte_array );
    // }

    // template< typename type_t >
    // generic_time_data::type_t* generic_time_data::get_array()
    // {
    //     return reinterpret_cast< type_t* >( f_byte_array );
    // }

    // template< typename type_t >
    // constexpr size_t generic_time_data::get_array_size() const
    // {
    //     return f_byte_array_size / sizeof(type_t);
    // }

} /* namespace psyllid */

#endif /* PSYLLID_GENERIC_TIME_DATA_HH_ */
