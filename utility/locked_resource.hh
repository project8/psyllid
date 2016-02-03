/*
 * locked_resource.hh
 *
 *  Created on: Feb 2, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_LOCKED_RESOURCE_HH_
#define PSYLLID_LOCKED_RESOURCE_HH_

#include <memory>
#include <mutex>

namespace psyllid
{

    template< class x_resource, class x_parent, class x_mutex = std::mutex, class x_lock = std::unique_lock< x_mutex > >
    class locked_resource
    {
        public:
            typedef std::shared_ptr< x_resource > resource_ptr_t;
            typedef x_resource resource_t;

            locked_resource() :
                f_resource(),
                f_lock()
            {}
            locked_resource( const locked_resource& ) = delete;
            locked_resource( locked_resource&& a_orig ) :
                f_resource( std::move( a_orig.f_resource ) ),
                f_lock( std::move( a_orig.f_lock ) )
            {}
            ~locked_resource() {}

            locked_resource& operator=( const locked_resource& ) = delete;
            locked_resource& operator=( locked_resource&& a_rhs )
            {
                f_resource = std::move( a_rhs.f_resource );
                a_rhs.f_resource.reset();
                f_lock = std::move( a_rhs.f_lock );
                a_rhs.f_lock.release();
                return *this;
            }

            resource_t* operator->() const { return f_resource.get(); }

        private:
            friend x_parent;
            locked_resource( resource_ptr_t a_resource, x_mutex& a_mutex ) :
                f_resource( a_resource ),
                f_lock( a_mutex )
            {}

            resource_ptr_t f_resource;
            x_lock f_lock;
    };

} /* namespace psyllid */


#endif /* PSYLLID_LOCKED_RESOURCE_HH_ */
