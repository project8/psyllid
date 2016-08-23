/*
 * packet_distributor.hh
 *
 *  Created on: Aug 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PACKET_DISTRIBUTOR_HH_
#define PSYLLID_PACKET_DISTRIBUTOR_HH_

#include "packet_buffer.hh"

#include <boost/container/flat_map.hpp>

namespace bcont = boost::container;

namespace psyllid
{

    class packet_distributor
    {
        public:
            packet_distributor();
            virtual ~packet_distributor();

        private:
            pb_iterator f_read_it;

            ::bcont::flat_map< unsigned, packet_buffer > f_write_buffers;
    };

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_DISTRIBUTOR_HH_ */
