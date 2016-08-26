/*
 * packet_eater.hh
 *
 *  Created on: Jul 23, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_PACKET_EATER_MANAGER_HH_
#define PSYLLID_PACKET_EATER_MANAGER_HH_

#include "singleton.hh"

#include "global_config.hh"
#include "param.hh"

#include <memory>
#include <map>
#include <string>

namespace psyllid
{
    class packet_eater;

    /*!
    @class packet_eater_manager
    @author N.S. Oblath

    @brief Global access to packet_eaters; Eaters are requested by network interface name

    @details
    Packet eater configuration is done by access to the global_config class.
    All configuration values are optional (defaults will be used if not provided; see the packet_eater class for details)
    To configure a packet the global configuration should contain a node looking like this:

        "packet-eaters" : {
            "ifc1" : {
                "timeout-sec": 2,
                "n-blocks": 100,
            }
        }
    */
	class packet_eater_manager : public scarab::singleton< packet_eater_manager >
	{
	    public:
	        typedef std::shared_ptr< packet_eater > pe_ptr;

	    public:
	        pe_ptr get_packet_eater( const std::string& a_net_interface );

	        void remove_packet_eater( const std::string& a_net_interface );

	    private:
	        std::map< std::string, pe_ptr > f_eaters;

	        scarab::param_node f_eater_config;

        protected:
            friend class scarab::singleton< packet_eater_manager >;
            friend class scarab::destroyer< packet_eater_manager >;
            packet_eater_manager();
            ~packet_eater_manager();
	};

} /* namespace psyllid */

#endif /* PSYLLID_PACKET_EATER_MANAGER_HH_ */
