/*
 * memory_block.hh
 *
 *  Created on: Nov 2, 2016
 *      Author: nsoblath
 */

#ifndef DATA_MEMORY_BLOCK_HH_
#define DATA_MEMORY_BLOCK_HH_

#include "member_variables.hh"

#include <cstdint>

namespace psyllid
{

    class memory_block
    {
        public:
            memory_block();
            virtual ~memory_block();

        public:
            mv_accessible( size_t, n_bytes );
            mv_accessible( uint8_t*, block );

            void resize( size_t a_n_bytes );
    };

} /* namespace psyllid */

#endif /* DATA_MEMORY_BLOCK_HH_ */
