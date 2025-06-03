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
            size_t data_type_size;
            uint32_t data_type_format;
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
                size_t data_type_size,
                uint32_t data_type_format,
                uint32_t data_type_sign
            );
            
            virtual ~generic_time_data();

            template< typename x_type >
            void initialize();

        public:
            typedef uint8_t byte_type;

            // functions to return the byte array
            const byte_type* get_byte_array() const;
            byte_type* get_byte_array();
            size_t get_byte_array_size() const;

            // functions to return the byte array, reinterpret cast
            template< typename x_type >
            const x_type* get_array() const;
            template< typename x_type >
            x_type* get_array();
            template< typename x_type >
            constexpr size_t get_array_size() const;
            
            // functions to return the data entries themselves, static cast
            template< typename x_type >
            const x_type get_at_as( size_t index ) const;

            // returning data characteristics
            mv_accessible( uint64_t, pkt_in_session );

            const current_data_format get_data_format() const;




        private:
            byte_type* f_byte_array;
            size_t f_byte_array_size;
            monarch3::M3Header f_egg_header;
            current_data_format f_data_format;

    };

    template<>
    void generic_time_data::initialize< uint8_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 1, 0, 0 } );
    }

    template<>
    void generic_time_data::initialize< uint16_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 2, 0, 0 } );
    }

    template<>
    void generic_time_data::initialize< uint32_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 4, 0, 0 } );
    }

    template<>
    void generic_time_data::initialize< uint64_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 8, 0, 0 } );
    }

    template<>
    void generic_time_data::initialize< int8_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 1, 0, 1 } );
    }

    template<>
    void generic_time_data::initialize< int16_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 2, 0, 1 } );
    }

    template<>
    void generic_time_data::initialize< int32_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 4, 0, 1 } );
    }

    template<>
    void generic_time_data::initialize< int64_t >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 8, 0, 1 } );
    }

    template<>
    void generic_time_data::initialize< float >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 4, 1, 0 } );
    }

    template<>
    void generic_time_data::initialize< double >()
    {
        generic_time_data::f_data_format = current_data_format( { 4096, 2, 8, 1, 0 } );
    }

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

    template< typename x_type >
    inline const x_type* generic_time_data::get_array() const
    {
        return reinterpret_cast< x_type* >( f_byte_array );
    }

    template< typename x_type >
    inline x_type* generic_time_data::get_array()
    {
        return reinterpret_cast< x_type* >( f_byte_array );
    }

    template< typename x_type >
    inline constexpr size_t generic_time_data::get_array_size() const
    {
        return f_byte_array_size / sizeof( x_type );
    }

    template< typename x_type >
    inline const x_type generic_time_data::get_at_as( size_t index ) const
    {
        if( f_data_format.data_type_format == 0 )
        {
            if( f_data_format.data_type_sign == 0 )
            {
                if( f_data_format.data_type_size == 1)
                {
                    return static_cast< x_type >( get_array< uint8_t >()[ index ] );
                }
                else if( f_data_format.data_type_size == 2 )
                {
                    return static_cast< x_type >( get_array< uint16_t >()[ index ] );
                }
                else if( f_data_format.data_type_size == 4 )
                {
                    return static_cast< x_type >( get_array< uint32_t >()[ index ] );
                }
                else if( f_data_format.data_type_size == 8 )
                {
                    return static_cast< x_type >( get_array< uint64_t >()[ index ] );
                }
            }
            else
            {
                if( f_data_format.data_type_size == 1)
                {
                    return static_cast< x_type >( get_array< int8_t >()[ index ] );
                }
                else if( f_data_format.data_type_size == 2 )
                {
                    return static_cast< x_type >( get_array< int16_t >()[ index ] );
                }
                else if( f_data_format.data_type_size == 4 )
                {
                    return static_cast< x_type >( get_array< int32_t >()[ index ] );
                }
                else if( f_data_format.data_type_size == 8 )
                {
                    return static_cast< x_type >( get_array< int64_t >()[ index ] );
                }
            }
        }
        else
        {
            if( f_data_format.data_type_size == 4 )
            {
                return static_cast< x_type >( get_array< float >()[ index ] );
            }
            else if( f_data_format.data_type_size == 8 )
            {
                return static_cast< x_type >( get_array< double >()[ index ] );
            }
        }
        
    }

    inline const current_data_format generic_time_data::get_data_format() const
    {
        return f_data_format;
    }

} /* namespace psyllid */

#endif /* PSYLLID_GENERIC_TIME_DATA_HH_ */
