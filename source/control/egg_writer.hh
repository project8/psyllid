/*
 * egg_writer.hh
 *
 *  Created on: Mar 14, 2017
 *      Author: nsoblath
 *
 */

#ifndef PSYLLID_EGG_WRITER_HH_
#define PSYLLID_EGG_WRITER_HH_

#include "monarch3_wrap.hh"

namespace psyllid
{
	/*!
     @class egg_writer
     @author N. S. Oblath

     @brief Base class for all writers.

     @details
     */
    class egg_writer
    {
        public:
            egg_writer();
            virtual ~egg_writer();

            virtual void prepare_to_write( monarch_wrap_ptr a_mw_ptr, header_wrap_ptr a_hw_ptr ) = 0;
    };

} /* namespace psyllid */

#endif /* PSYLLID_EGG_WRITER_HH_ */
