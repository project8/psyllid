/*
 * constants.hh
 *
 *  Created on: Jan 23, 2015
 *      Author: nsoblath
 */

#ifndef PSYLLID_CONSTANTS_HH_
#define PSYLLID_CONSTANTS_HH_

// Executable return constants

#define RETURN_SUCCESS 1
#define RETURN_ERROR -1
#define RETURN_CANCELED -2
#define RETURN_REVOKED -3

namespace psyllid
{
    // Combined date & time, according to the ISO 8601 standard: e.g. 2015-01-31T22:35:58Z
    char date_time_format[] = "%Y-%m-%dT%H:%M:%SZ";
}

#endif /* PSYLLID_CONSTANTS_HH_ */
