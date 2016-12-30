/*
 * byte_swap.hh
 *
 *  Created on: Jan 29, 2016
 *      Author: nsoblath
 */

#ifndef UTILITY_BYTE_SWAP_HH_
#define UTILITY_BYTE_SWAP_HH_

#ifdef __APPLE__
#include <machine/endian.h>
// this is the only function currently used in psyllid; more could be added as necessary
#define be64toh(x) ntohll(x)
#else
#include <endian.h>
#endif

#endif /* UTILITY_BYTE_SWAP_HH_ */
