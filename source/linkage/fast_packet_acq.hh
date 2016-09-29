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

    @brief Fast-packet-acquisition system for a particular network interface

    @details
    Each fast_packet_acq instance applies to a single network interface (e.g. eth0).
    All configuration values are optional (defaults will be used if not provided; see the packet_eater class for details)
    To configure a fast-packet-acq for interface eth1, the configuration might look like this:

        eth1:
            - timeout-sec: 2
            - n-blocks: 100

    */
	class fast_packet_acq : public midge::bystander
	{
	    public:
            fast_packet_acq( const std::string& a_interface = "" );
            virtual ~fast_packet_acq();

            pe_ptr eater();
            pd_ptr distributor();

            const std::string& net_interface() const;

	    public:
            void initialize(); // set interface before calling this
            bool activate_port( unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size = 100 ); // initialize before calling this

            void execute();

            void finalize();

        private:
            bool f_is_initialized;
            bool f_is_running;

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

    inline const std::string& fast_packet_acq::net_interface() const
    {
        return f_name;
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
