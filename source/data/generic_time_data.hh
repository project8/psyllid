/*
 * time_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_GENERIC_TIME_DATA_HH_
#define PSYLLID_GENERIC_TIME_DATA_HH_

#include "roach_packet.hh"

#include "member_variables.hh"


namespace psyllid
{

    class generic_time_data
    {
        public:
            generic_time_data();
            virtual ~generic_time_data() = default;

        public:
            typedef uint8_t byte_type;

            const byte_type* get_byte_array() const;
            byte_type* get_byte_array();
            size_t get_byte_array_size() const;

            template< typename type_t >
            const type_t* get_array() const;
            template< typename type_t >
            type_t* get_array();
            template< typename type_t >
            constexpr size_t get_array_size() const;

            mv_accessible( uint64_t, pkt_in_session );

        private:
            byte_type* f_byte_array;
            size_t f_byte_array_size;
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

    template< typename type_t >
    const generic_time_data::type_t* generic_time_data::get_array() const
    {
        return reinterpret_cast< type_t* >( f_byte_array );
    }

    template< typename type_t >
    generic_time_data::type_t* generic_time_data::get_array()
    {
        return reinterpret_cast< type_t* >( f_byte_array );
    }

    template< typename type_t >
    constexpr size_t generic_time_data::get_array_size() const
    {
        return f_byte_array_size / sizeof(type_t);
    }

} /* namespace psyllid */

#endif /* PSYLLID_GENERIC_TIME_DATA_HH_ */
