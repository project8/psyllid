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

    DECLARE_PRESET( roach_config );

	DECLARE_PRESET( streaming_1ch );
#ifdef __linux__
	DECLARE_PRESET( streaming_1ch_fpa );
#endif

	DECLARE_PRESET( fmask_trigger_1ch);
} /* namespace psyllid */

#endif /* PSYLLID_ROACH_CONFIG_HH_ */
