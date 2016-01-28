/*
 * persistent_store.hh
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PERSISTENT_STORE_HH_
#define PSYLLID_PERSISTENT_STORE_HH_

#include "psyllid_error.hh"

#include <map>
#include <memory>
#include <string>

namespace psyllid
{

    class persistent_store
    {
        public:
            class label_in_use : public error
            {
                public:
                    label_in_use( const std::string& a_label ) { f_message = "The label <" + a_label + "> is already in use"; }
                    virtual ~label_in_use() {}
            };

            class label_not_found : public error
            {
                public:
                    label_not_found( const std::string& a_label ) { f_message = "The label <" + a_label + "> was not found"; }
            };

            class wrong_type : public error
            {
                public:
                    wrong_type() { f_message = "Incorrect type requested"; }
                    virtual ~wrong_type() {}
            };

        public:
            persistent_store();
            virtual ~persistent_store();

        public:
            template< typename x_type >
            void store( const std::string& a_label, std::shared_ptr< x_type > an_item ); // will throw label_in_use if the label is already in use

            template< typename x_type >
            std::shared_ptr< x_type > retrieve( const std::string& a_label ); // will throw label_not_found if the label isn't present, and wrong_type if the type supplied is wrong for the given label

            void dump( const std::string& a_label ); // does not throw

            bool has( const std::string& a_label ) const; // does not throw

        private:
            class storable
            {
                public:
                    storable() {}
                    storable( const storable& ) = delete;

                    virtual ~storable() {}

                    template< typename x_type >
                    std::shared_ptr< x_type > retrieve() const
                    {
                        _storable< x_type >* t_derived_storable = dynamic_cast< _storable< x_type >* >( this );
                        if( t_derived_storable == nullptr )
                        {
                            throw wrong_type();
                        }
                        return t_derived_storable->_retrieve();
                    }
            };

            template< typename x_type >
            class _storable : public storable
            {
                public:
                    typedef std::shared_ptr< x_type > stored_ptr_t;

                public:
                    _storable() = delete;
                    _storable( stored_ptr_t an_item ) :
                        storable(),
                        f_stored( an_item )
                    {}
                    _storable( const _storable& ) = delete;

                    virtual ~_storable() {}

                    virtual stored_ptr_t _retrieve() const
                    {
                        return f_stored;
                    }

                private:
                    stored_ptr_t f_stored;
            };

            typedef std::map< std::string, std::unique_ptr< storable > > storage_t;
            typedef storage_t::const_iterator storage_cit_t;
            typedef storage_t::iterator storage_it_t;

            storage_t f_storage;
    };

    template< typename x_type >
    void persistent_store::store( const std::string& a_label, std::shared_ptr< x_type > an_item )
    {
        storage_it_t t_item_it = f_storage.find( a_label );
        if( t_item_it != f_storage.end() )
        {
            throw label_in_use( a_label );
        }

        f_storage.emplace( a_label, new _storable< x_type >( an_item ) );
        return;
    }

    template< typename x_type >
    std::shared_ptr< x_type > persistent_store::retrieve( const std::string& a_label )
    {
        storage_it_t t_item_it = f_storage.find( a_label );
        if( t_item_it == f_storage.end() )
        {
            throw label_not_found( a_label );
        }

        try
        {
            std::shared_ptr< x_type > t_item = t_item_it->second->retrieve< x_type >();
            f_storage.erase( t_item_it );
            return t_item;
        }
        catch(...)
        {
            throw;
        }
    }

    inline bool persistent_store::has( const std::string& a_label ) const
    {
        return f_storage.find( a_label ) != f_storage.end();
    }

} /* namespace psyllid */

#endif /* PSYLLID_PERSISTENT_STORE_HH_ */
