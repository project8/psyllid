/*
 * fpa_factory.hh
 *
 *  Created on: Sep 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_FPA_FACTORY_HH_
#define PSYLLID_FPA_FACTORY_HH_

#include "fast_packet_acq.hh"

#include "singleton.hh"

#include <map>
#include <memory>
#include <string>

namespace psyllid
{
    typedef std::shared_ptr< fast_packet_acq > fpa_ptr;

    class fpa_factory : public scarab::singleton< fpa_factory >
    {
        public:
            fpa_ptr get_fpa( const std::string& a_interface );

        protected:
            allow_singleton_access( fpa_factory );
            fpa_factory();
            virtual ~fpa_factory();

        private:
            typedef std::map< std::string, fpa_ptr > fpa_map;
            typedef fpa_map::iterator fpa_map_it;
            fpa_map f_fpa_map;
    };

} /* namespace psyllid */

#endif /* PSYLLID_FPA_FACTORY_HH_ */
