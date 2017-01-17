/*
 * stream_preset.hh
 *
 *  Created on: Jan 27, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_STREAM_PRESET_HH_
#define PSYLLID_STREAM_PRESET_HH_

#include "factory.hh"

#include <map>
#include <set>
#include <string>

namespace scarab
{
    class param_node;
}

namespace psyllid
{
    class node_manager;

    class stream_preset
    {
        public:
            typedef std::map< std::string, std::string > nodes_t;
            typedef std::set< std::string > connections_t;

        public:
            stream_preset();
            stream_preset( const std::string& a_type );
            stream_preset( const stream_preset& a_orig );
            virtual ~stream_preset();

            stream_preset& operator=( const stream_preset& a_rhs );

            const nodes_t& get_nodes() const;
            const connections_t& get_connections() const;

        protected:
            void node( const std::string& a_type, const std::string& a_name );
            void connection( const std::string& a_conn );

        protected:
            std::string f_type;
            nodes_t f_nodes;
            connections_t f_connections;
    };


    class node_config_runtime_preset : public stream_preset
    {
        public:
            node_config_runtime_preset();
            node_config_runtime_preset( const std::string& a_name );
            node_config_runtime_preset( const node_config_runtime_preset& a_orig );
            virtual ~node_config_runtime_preset();

            node_config_runtime_preset& operator=( const node_config_runtime_preset& a_rhs );

        public:
            static bool add_preset( const scarab::param_node* a_preset_node );

        protected:
            static std::map< std::string, node_config_runtime_preset> s_runtime_presets;

    };




    inline const stream_preset::nodes_t& stream_preset::get_nodes() const
    {
        return f_nodes;
    }

    inline const stream_preset::connections_t& stream_preset::get_connections() const
    {
        return f_connections;
    }


} /* namespace psyllid */



#define DECLARE_PRESET( preset_class ) \
	class preset_class : public stream_preset \
    { \
        public: \
    		preset_class( const std::string& a_type ); \
            virtual ~preset_class() {}; \
    };

#define REGISTER_PRESET( preset_class, preset_type ) \
        static ::scarab::registrar< ::psyllid::stream_preset, preset_class, const std::string& > s_stream_preset_##preset_class##_registrar( preset_type );


#endif /* CONTROL_STREAM_PRESET_HH_ */
