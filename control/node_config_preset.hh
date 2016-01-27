/*
 * node_config_preset.hh
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_CONFIG_PRESET_HH_
#define PSYLLID_NODE_CONFIG_PRESET_HH_

#include <map>
#include <set>
#include <string>

namespace psyllid
{
    class node_manager;

    class node_config_preset
    {
        public:
            node_config_preset();
            virtual ~node_config_preset();

            void set_nodes( node_manager* a_manager ) const;
            void set_connections( node_manager* a_manager ) const;

        protected:
            typedef std::map< std::string, std::string > nodes_t;
            typedef std::set< std::string > connections_t;

        private:
            nodes_t f_nodes;
            connections_t f_connections;
    };

} /* namespace psyllid */

#endif /* CONTROL_NODE_CONFIG_PRESET_HH_ */
