/*
 * psyllid_version.hh
 *
 *  Created on: Mar 20, 2013
 *      Author: nsoblath
 */

#ifndef PSYLLID_VERSION_HH_
#define PSYLLID_VERSION_HH_

#include "dripline_version.hh"

namespace psyllid
{
    class version : public scarab::version_semantic
    {
        public:
            version();
            ~version();
    };

} // namespace psyllid

#endif /* PSYLLID_VERSION_HH_ */
