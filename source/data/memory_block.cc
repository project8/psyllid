/*
 * memory_block.cc
 *
 *  Created on: Nov 2, 2016
 *      Author: nsoblath
 */

#include "memory_block.hh"

#include <cstdlib>

namespace psyllid
{

    memory_block::memory_block() :
            f_n_bytes( 0 ),
            f_n_bytes_used( 0 ),
            f_block( nullptr )
    {
    }

    memory_block::~memory_block()
    {
        if( f_n_bytes != 0 ) free( (void*)f_block );
    }

    void memory_block::resize( size_t a_n_bytes )
    {
        if( a_n_bytes == f_n_bytes ) return;
        if( f_n_bytes != 0 ) ::free( (void*)f_block );
        if( a_n_bytes != 0 ) f_block = (uint8_t*)::malloc( a_n_bytes );
        else f_block = nullptr;
        return;
    }

} /* namespace psyllid */
