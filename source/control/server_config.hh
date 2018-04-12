/*
 * server_config.hh
 *
 *  Created on: Nov 4, 2013
 *      Author: nsoblath
 */

#ifndef PSYLLID_SERVER_CONFIG_HH_
#define PSYLLID_SERVER_CONFIG_HH_

#include "param.hh"

namespace psyllid
{

    class server_config : public scarab::param_node
    {
        public:
            server_config();
            virtual ~server_config();
    };

} /* namespace psyllid */
#endif /* PSYLLID_SERVER_CONFIG_HH_ */
