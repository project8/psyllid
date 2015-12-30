/*
 * time_data.hh
 *
 *  Created on: Dec 28, 2015
 *      Author: nsoblath
 */

#ifndef DATA_TIME_DATA_HH_
#define DATA_TIME_DATA_HH_

#include "macros.hh"
#include "types.hh"

#include <vector>
using std::vector;

#include <memory>
using std::unique_ptr;

namespace psyllid
{

    class time_data
    {
        public:
            typedef int8_t value_type;
            typedef vector< value_type > array_type;

        public:
            time_data();
            virtual ~time_data();

        public:
            accessible( midge::count_t, id );
            referrable( unique_ptr< array_type >, array );

        public:
            midge::count_t get_array_n_bytes() const;
    };


    inline midge::count_t time_data::get_array_n_bytes() const
    {
        return f_array->size() * sizeof( value_type );
    }

} /* namespace psyllid */

#endif /* DATA_TIME_DATA_HH_ */
