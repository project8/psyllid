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
#include <cstddef> // for size_t

namespace psyllid
{

    class memory_block
    {
        public:
            memory_block();
            virtual ~memory_block();

        public:
            void resize( size_t a_n_bytes );
            uint8_t* block();
            const uint8_t* block() const;

            mv_accessible( size_t, n_bytes );
            mv_accessible( size_t, n_bytes_used );

        private:
            uint8_t* f_block;
    };

    inline uint8_t* memory_block::block()
    {
        return f_block;
    }

    inline const uint8_t* memory_block::block() const
    {
        return f_block;
    }

} /* namespace psyllid */

#endif /* DATA_MEMORY_BLOCK_HH_ */
