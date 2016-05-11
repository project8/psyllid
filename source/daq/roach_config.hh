/*
 * roach_config.hh
 *
 *  Created on: Feb 4, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_ROACH_CONFIG_HH_
#define PSYLLID_ROACH_CONFIG_HH_

#include "node_config_preset.hh"

namespace psyllid
{

    class roach_config : public node_config_preset
    {
        public:
            roach_config();
            virtual ~roach_config();
    };

} /* namespace psyllid */

#endif /* PSYLLID_ROACH_CONFIG_HH_ */
