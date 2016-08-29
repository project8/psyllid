/*
 * fast_packet_acq_manager.hh
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_FAST_PACKET_ACQ_MANAGER_HH_
#define PSYLLID_FAST_PACKET_ACQ_MANAGER_HH_

#include "singleton.hh"

#include "global_config.hh"
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

    struct fpa_route
    {
        pe_ptr f_eater;
        pd_ptr f_distributor;
    };

    /*!
    @class fast_packet_acq_manager
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
	class fast_packet_acq_manager : public scarab::singleton< fast_packet_acq_manager >
	{
	    public:
	        bool activate_port( const std::string& a_net_interface, unsigned a_port, pb_iterator& a_iterator, unsigned a_buffer_size = 100 );

	        void remove_interface( const std::string& a_net_interface );

	    private:
	        typedef std::map< std::string, fpa_route > route_map;
	        route_map f_routes;

	        scarab::param_node f_config;

	    public:
	        void execute();

        protected:
            friend class scarab::singleton< fast_packet_acq_manager >;
            friend class scarab::destroyer< fast_packet_acq_manager >;
            fast_packet_acq_manager();
            ~fast_packet_acq_manager();
	};

} /* namespace psyllid */

#endif /* PSYLLID_FAST_PACKET_ACQ_MANAGER_HH_ */
