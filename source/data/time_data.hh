/*
 * time_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_TIME_DATA_HH_
#define PSYLLID_TIME_DATA_HH_

#include "roach_packet.hh"

#include "member_variables.hh"


namespace psyllid
{

    class time_data : public roach_packet_data
    {
        public:
            time_data();
            virtual ~time_data();

        public:
            typedef int8_t iq_t[2];

            const iq_t* get_array() const;
            iq_t* get_array();
            size_t get_array_size() const;

            mv_accessible( uint64_t, pkt_in_session );

        private:
            iq_t* f_array;
            size_t f_array_size;
    };

    inline const time_data::iq_t* time_data::get_array() const
    {
        return f_array;
    }

    inline time_data::iq_t* time_data::get_array()
    {
        return f_array;
    }

    inline size_t time_data::get_array_size() const
    {
        return f_array_size;
    }

/*
    class time_data
    {
        public:
            typedef int8_t value_type;
            typedef std::vector< value_type > array_type;

        public:
            time_data();
            virtual ~time_data();

        public:
            mv_accessible( uint64_t, id );
            mv_referrable( std::unique_ptr< array_type >, array );

        public:
            uint64_t get_array_n_bytes() const;
    };


    inline uint64_t time_data::get_array_n_bytes() const
    {
        return f_array->size() * sizeof( value_type );
    }
*/

} /* namespace psyllid */

#endif /* PSYLLID_TIME_DATA_HH_ */
