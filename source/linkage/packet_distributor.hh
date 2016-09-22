/*
 * packet_distributor.hh
 *
 *  Created on: Aug 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PACKET_DISTRIBUTOR_HH_
#define PSYLLID_PACKET_DISTRIBUTOR_HH_

#include "packet_buffer.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <boost/container/flat_map.hpp>

namespace bcont = boost::container;

namespace psyllid
{

    /*!
    @class packet_distributor
    @author N.S. Oblath

    @brief Accepts IP packets on a single buffer and distributes UDP packets to multiple buffers according to port number.

    @details
    */
    class packet_distributor : public scarab::cancelable
    {
        public:
            packet_distributor();
            virtual ~packet_distributor();

            bool open_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size = 100 );
            bool close_port( unsigned a_port );

            void initialize();

            void execute();

            void set_ip_buffer( packet_buffer* a_buffer );

        private:
            bool distribute_packet();

            packet_buffer* f_ip_buffer;

            pb_iterator f_ip_pkt_iterator;

            struct udp_buffer
            {
                //udp_buffer() {};
                ~udp_buffer() {};
                packet_buffer f_buffer;
                pb_iterator f_iterator;
            };
            typedef ::bcont::flat_map< unsigned, udp_buffer > buffer_map;

            buffer_map f_udp_buffers;
    };

    inline void packet_distributor::set_ip_buffer( packet_buffer* a_buffer )
    {
        f_ip_buffer = a_buffer;
    }

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_DISTRIBUTOR_HH_ */
