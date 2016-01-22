/*
 * time_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_TIME_DATA_HH_
#define DATA_TIME_DATA_HH_

#include "member_variables.hh"

#include <vector>
#include <memory>

namespace psyllid
{

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

} /* namespace psyllid */

#endif /* DATA_TIME_DATA_HH_ */
