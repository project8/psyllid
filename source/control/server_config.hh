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
	/*!
	 @class server_config
	 @author N. S. Oblath

	 @brief Contains default server configuration

	 @details
	 Contains default configurations for:
	 - broker
	 - queue
	 - auth-file
	 - slack-queue
	 - activate-at-startup
	 - n-files
	 - duration
	 - max-file-size-mb

	 These default configurations, together with the configurations from the command line and the config-file, are passed to scarab::configurator by the psyllid executable.
	 The configurator combines them and extracts the final psyllid configuration which is then passed to the run_server during initialization.
	 */
    class server_config : public scarab::param_node
    {
        public:
            server_config();
            virtual ~server_config();
    };

} /* namespace psyllid */
#endif /* PSYLLID_SERVER_CONFIG_HH_ */
