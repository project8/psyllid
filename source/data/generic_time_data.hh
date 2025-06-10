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
    

    class generic_time_data
    {
        private:
            enum DataType
            {
                DT_UINT8, 
                DT_UINT16,
                DT_UINT32,
                DT_UINT64,
                DT_INT8,
                DT_INT16,
                DT_INT32,
                DT_INT64,
                DT_FLOAT,
                DT_DOUBLE
            };

            struct current_data_format
            {
                size_t record_size;
                size_t sample_size;
                DataType data_type;
            };

        public:
            // We start by wanting the compiler to catch any and all possible problems. If the program compiles, it should run, up to numeric factors. 
            // We want type specific constructors, where the template type is that of the stored data. 
            // The egg reader should then, having determined what the type of the data in the egg file is, produce a generic_time_data with that type in it. 
            // Does it make the most sense to propagate the egg header? 
            // I think so, that should have all information in it regarding how the simulation was made. 
            // If its not in there, then the 'converter node' should have some parameter that uses that information. 
            generic_time_data();
            generic_time_data(
                size_t record_size,
                size_t sample_size,
                DataType data_type
            );
            generic_time_data(
                monarch3::M3Header egg_header
            );
            generic_time_data(
                size_t record_size,
                size_t sample_size,
                DataType data_type,
                monarch3::M3Header egg_header
            );
            virtual ~generic_time_data();

            generic_time_data& operator=( const generic_time_data& a_orig );
            generic_time_data& operator=( generic_time_data&& a_orig );

            template< typename x_type >
            void initialize( size_t record_size, size_t sample_size );

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

            size_t get_data_size();
            uint32_t get_data_format();
            uint32_t get_data_signed();

            // manipulating egg_header
            const monarch3::M3Header get_header() const;
            void set_header( monarch3::M3Header new_header );

        private: 
            void initialize();
            

        private:
            byte_type* f_byte_array;
            size_t f_byte_array_size;
            monarch3::M3Header f_egg_header;
            current_data_format f_data_format;
            bool f_has_data;


    };

    inline size_t generic_time_data::get_data_size()
    {
        switch( f_data_format.data_type )
        {
            case generic_time_data::DT_UINT8: return sizeof( uint8_t );
            case generic_time_data::DT_UINT16: return sizeof( uint16_t );
            case generic_time_data::DT_UINT32: return sizeof( uint32_t );
            case generic_time_data::DT_UINT64: return sizeof( uint64_t );
            case generic_time_data::DT_INT8: return sizeof( int8_t );
            case generic_time_data::DT_INT16: return sizeof( int16_t );
            case generic_time_data::DT_INT32: return sizeof( int32_t );
            case generic_time_data::DT_INT64: return sizeof( int64_t );
            case generic_time_data::DT_FLOAT: return sizeof( float );
            case generic_time_data::DT_DOUBLE: return sizeof( double );
            default: return 0; // SOME KIND OF ERROR
        }
    }

    inline uint32_t generic_time_data::get_data_format()
    {
        switch( f_data_format.data_type )
        {
            case generic_time_data::DT_UINT8: return 0;
            case generic_time_data::DT_UINT16: return 0;
            case generic_time_data::DT_UINT32: return 0;
            case generic_time_data::DT_UINT64: return 0;
            case generic_time_data::DT_INT8: return 0;
            case generic_time_data::DT_INT16: return 0;
            case generic_time_data::DT_INT32: return 0;
            case generic_time_data::DT_INT64: return 0;
            case generic_time_data::DT_FLOAT: return 1;
            case generic_time_data::DT_DOUBLE: return 1;
            default: return 0; // SOME KIND OF ERROR
        }
    }

    inline uint32_t generic_time_data::get_data_signed()
    {
        switch( f_data_format.data_type )
        {
            case generic_time_data::DT_UINT8: return 0;
            case generic_time_data::DT_UINT16: return 0;
            case generic_time_data::DT_UINT32: return 0;
            case generic_time_data::DT_UINT64: return 0;
            case generic_time_data::DT_INT8: return 1;
            case generic_time_data::DT_INT16: return 1;
            case generic_time_data::DT_INT32: return 1;
            case generic_time_data::DT_INT64: return 1;
            case generic_time_data::DT_FLOAT: return 1;
            case generic_time_data::DT_DOUBLE: return 1;
            default: return 0; // SOME KIND OF ERROR
        }
    }

    template< typename x_type >
    inline const x_type generic_time_data::get_at_as( size_t index ) const
    {
        switch( f_data_format.data_type )
        {
            case generic_time_data::DT_UINT8:  return static_cast< x_type >( get_array< uint8_t >()[ index ] );
            case generic_time_data::DT_UINT16: return static_cast< x_type >( get_array< uint16_t >()[ index ] );
            case generic_time_data::DT_UINT32: return static_cast< x_type >( get_array< uint32_t >()[ index ] );
            case generic_time_data::DT_UINT64: return static_cast< x_type >( get_array< uint64_t >()[ index ] );
            case generic_time_data::DT_INT8:   return static_cast< x_type >( get_array< int8_t >()[ index ] );
            case generic_time_data::DT_INT16:  return static_cast< x_type >( get_array< int16_t >()[ index ] );
            case generic_time_data::DT_INT32:  return static_cast< x_type >( get_array< int32_t >()[ index ] );
            case generic_time_data::DT_INT64:  return static_cast< x_type >( get_array< int64_t >()[ index ] );
            case generic_time_data::DT_FLOAT:  return static_cast< x_type >( get_array< float >()[ index ] );
            case generic_time_data::DT_DOUBLE: return static_cast< x_type >( get_array< double >()[ index ] );
            default: return 0; // SOME KIND OF ERROR
        }
    }

    inline const generic_time_data::current_data_format generic_time_data::get_data_format() const
    {
        return f_data_format;
    }

    inline const monarch3::M3Header generic_time_data::get_header() const
    {
        return f_egg_header;
    }

    inline void generic_time_data::set_header( monarch3::M3Header new_header )
    {
        f_egg_header = new_header;
    }

    void generic_time_data::initialize()
    {
        if( f_has_data )
        {
            delete [] f_byte_array;
        }
        f_byte_array_size = f_data_format.record_size * f_data_format.sample_size * generic_time_data::get_data_size();
        f_byte_array = new byte_type[ f_byte_array_size ];
        f_has_data = true;
    }

    template<>
    void generic_time_data::initialize< uint8_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_UINT8 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< uint16_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_UINT16 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< uint32_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_UINT32 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< uint64_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_UINT64 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< int8_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_INT8 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< int16_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_INT16 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< int32_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_INT32 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< int64_t >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_INT64 } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< float >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_FLOAT } );
        generic_time_data::initialize();
    }

    template<>
    void generic_time_data::initialize< double >( size_t record_size, uint32_t sample_size )
    {
        generic_time_data::f_data_format = generic_time_data::current_data_format( { record_size, sample_size , DT_DOUBLE } );
        generic_time_data::initialize();
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

    

} /* namespace psyllid */

#endif /* PSYLLID_GENERIC_TIME_DATA_HH_ */
