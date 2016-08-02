/*
 * packet_eater.hh
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#ifndef SOURCE_UTILITY_PACKET_EATER_HH_
#define SOURCE_UTILITY_PACKET_EATER_HH_

#include <linux/if_packet.h>

namespace psyllid
{

	struct block_desc
	{
		uint32_t f_version;
		uint32_t f_offset_to_priv;
		struct tpacket_hdr_v1 f_h1;
	};

	struct receive_ring
	{
		struct iovec* f_rd;
		uint8_t* f_map;
		struct tpacket_req3 f_req;
	};

	class packet_eater
	{
		public:
			packet_eater( unsigned a_timeout_sec = 0 );
			virtual ~packet_eater();

			void execute();

		private:
			void walk_block();
			void flush_block();

            int f_socket;

			receive_ring* f_ring;

			sockaddr_ll* f_address;

	};

} /* namespace psyllid */

#endif /* SOURCE_UTILITY_PACKET_EATER_HH_ */
