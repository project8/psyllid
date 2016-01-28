/*
 * persistent_store.cc
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#include "persistent_store.hh"

namespace psyllid
{

    persistent_store::persistent_store() :
            f_storage()
    {
    }

    persistent_store::~persistent_store()
    {
    }

    void persistent_store::dump( const std::string& a_label )
    {
        storage_it_t t_item_it = f_storage.find( a_label );
        if( t_item_it != f_storage.end() )
        {
            f_storage.erase( t_item_it );
        }
        return;
    }


} /* namespace psyllid */
