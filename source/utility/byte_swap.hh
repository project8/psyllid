/*
 * byte_swap.hh
 *
 *  Created on: Jan 29, 2016
 *      Author: nsoblath
 */

#ifndef UTILITY_BYTE_SWAP_HH_
#define UTILITY_BYTE_SWAP_HH_


#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define bswap_16 OSSwapInt16
#define bswap_32 OSSwapInt32
#define bswap_64 OSSwapInt64
#else
#include <byteswap.h>
#endif



#endif /* UTILITY_BYTE_SWAP_HH_ */
