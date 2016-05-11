/*
 * node_config_preset.hh
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_CONFIG_PRESET_HH_
#define PSYLLID_NODE_CONFIG_PRESET_HH_

#include "factory.hh"

#include <map>
#include <set>
#include <string>

namespace psyllid
{
    class node_manager;

    class node_config_preset
    {
        public:
            typedef std::map< std::string, std::string > nodes_t;
            typedef std::set< std::string > connections_t;

        public:
            node_config_preset();
            virtual ~node_config_preset();

            const nodes_t& get_nodes() const;
            const connections_t& get_connections() const;

        protected:
            void node( const std::string& a_type, const std::string& a_name );
            void connection( const std::string& a_conn );

        private:
            nodes_t f_nodes;
            connections_t f_connections;

    };

    inline const node_config_preset::nodes_t& node_config_preset::get_nodes() const
    {
        return f_nodes;
    }

    inline const node_config_preset::connections_t& node_config_preset::get_connections() const
    {
        return f_connections;
    }

} /* namespace psyllid */

#define REGISTER_PRESET( preset_class, preset_name ) \
        static ::scarab::registrar< ::psyllid::node_config_preset, preset_class > s_node_config_preset_##preset_class##_registrar( preset_name );

#endif /* CONTROL_NODE_CONFIG_PRESET_HH_ */
