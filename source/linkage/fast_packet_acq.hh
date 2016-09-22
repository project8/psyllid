/*
 * fast_packet_acq.hh
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_FAST_PACKET_ACQ_HH_
#define PSYLLID_FAST_PACKET_ACQ_HH_

#include "bystander.hh"

#include "node_builder.hh"

#include "member_variables.hh"
#include "param.hh"

#include <memory>
#include <map>
#include <string>

namespace psyllid
{
    class packet_eater;
    class packet_distributor;
    class pb_iterator;

    typedef std::shared_ptr< packet_eater > pe_ptr;
    typedef std::shared_ptr< packet_distributor > pd_ptr;

    /*!
    @class fast_packet_acq
    @author N.S. Oblath

    @brief Global access to fast-packet-acquisition system; Eaters and distributors are requested by network interface name

    @details
    Fast-packet-acquisition configuration is done by access to the global_config class.
    All configuration values are optional (defaults will be used if not provided; see the packet_eater class for details)
    To configure a packet the global configuration should contain a node looking like this:

        "fast-packet-acq" : {
            "ifc1" : {
                "timeout-sec": 2,
                "n-blocks": 100,
            }
        }
    */
	class fast_packet_acq : public midge::bystander
	{
	    public:
            fast_packet_acq();
            virtual ~fast_packet_acq();

            pe_ptr eater();
            pd_ptr distributor();

            mv_referrable( std::string, net_interface );

	    public:
            void initialize();
            bool activate_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size = 100 );

            void execute();

            void finalize();

        private:
	        pe_ptr f_eater;
	        pd_ptr f_distributor;

        private:
            virtual void do_cancellation();
            virtual void do_reset_cancellation();
	};

    inline pe_ptr fast_packet_acq::eater()
    {
        return f_eater;
    }
    inline pd_ptr fast_packet_acq::distributor()
    {
        return f_distributor;
    }



    class fast_packet_acq_builder : public _node_builder< fast_packet_acq >
    {
        public:
            fast_packet_acq_builder();
            virtual ~fast_packet_acq_builder();

        private:
            virtual void apply_config( fast_packet_acq* a_node, const scarab::param_node& a_config );
    };

} /* namespace psyllid */

#endif /* PSYLLID_FAST_PACKET_ACQ_HH_ */
