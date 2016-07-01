/*
 * freq_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_FREQ_DATA_HH_
#define PSYLLID_FREQ_DATA_HH_

#include "roach_packet.hh"

#include "member_variables.hh"


namespace psyllid
{

    class freq_data : public roach_packet_data
    {
        public:
            freq_data();
            virtual ~freq_data();

        public:
            typedef int8_t iq_t[2];

            const iq_t* get_array() const;
            size_t get_array_size() const;

            mv_accessible( uint64_t, pkt_in_session );

        private:
            iq_t* f_array;
            size_t f_array_size;
    };

    inline const freq_data::iq_t* freq_data::get_array() const
    {
        return f_array;
    }

    inline size_t freq_data::get_array_size() const
    {
        return f_array_size;
    }

    /*
    class freq_data
    {
        public:
            freq_data();
            virtual ~freq_data();

        public:
            mv_accessible( uint64_t, id );
            mv_referrable( std::unique_ptr< std::vector< double > >, array );
    };
    */

} /* namespace psyllid */

#endif /* PSYLLID_FREQ_DATA_HH_ */
