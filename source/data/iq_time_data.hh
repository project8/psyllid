/*
 * iq_time_data.hh
 *
 *  Created on: Mar 21, 2025
 *      Author: pkolbeck
 */

#ifndef PSYLLID_IQ_TIME_DATA_HH_
#define PSYLLID_IQ_TIME_DATA_HH_

#include "roach_packet.hh"

#include "member_variables.hh"

#include <cinttypes>
#include <cstddef> // for size_t

// number of samples in the roach_packet f_data array
//#define PAYLOAD_SIZE 8192 // 1KB

namespace psyllid 
{

    class iq_time_data
    {
        public: 
            iq_time_data();
            iq_time_data( const iq_time_data& a_orig );
            iq_time_data( iq_time_data&& a_orig );
            virtual ~iq_time_data();

            iq_time_data& operator=( const iq_time_data& a_orig );
            iq_time_data& operator=( iq_time_data&& a_orig );

        public: 
            typedef int8_t iq_t[2];

            const iq_t* get_array() const;
            iq_t* get_array();
            const iq_t* get_array_ptr() const;
            iq_t* get_array_ptr();
            void set_array_ptr( iq_t* array );
            size_t get_array_size() const;
            const bool get_has_data() const;

            mv_accessible( uint64_t, pkt_in_session );
            mv_accessible( bool, owns_data );

        public:
            void initialize( size_t a_size, iq_time_data::iq_t* a_external_array);
        
        private:
            iq_t* f_array;
//            iq_t f_array[ PAYLOAD_SIZE/2 ];
            size_t f_array_size;

    };

    inline const iq_time_data::iq_t* iq_time_data::get_array() const
    {
        return f_array;
    }

    inline iq_time_data::iq_t* iq_time_data::get_array()
    {
        return f_array;
    }

    inline size_t iq_time_data::get_array_size() const
    {
        return f_array_size;
    }


}

#endif /* PSYLLID_IQ_TIME_DATA_HH_ */
