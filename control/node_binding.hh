/*
 * node_binding.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_BINDING_HH_
#define PSYLLID_NODE_BINDING_HH_

#include "member_variables.hh"
#include "param.hh"

namespace midge
{
    class node;
}

namespace psyllid
{

    class node_binding
    {
        public:
            node_binding( const std::string& a_name );
            virtual ~node_binding();

            void configure( const scarab::param_node& a_node );
            void replace_config( const scarab::param_node& a_node );

            void dump_config( scarab::param_node& a_node );

            virtual midge::node* build() = 0;

            mv_referrable( std::string, name );

        private:
            scarab::param_node f_config;
    };

    template< class x_node_type >
    class _node_binding : public node_binding
    {
        public:
            _node_binding( const std::string& a_name );
            virtual ~_node_binding();

            virtual midge::node* build();

        private:
            virtual void apply_config( x_node_type* a_node ) = 0;

    };

    template< class x_node_type >
    midge::node* _node_binding< x_node_type >::build()
    {
        x_node_type* t_node = new x_node_type();

        apply_config( t_node );
        t_node->set_name( f_name );

        return t_node;
    }


} /* namespace psyllid */

#endif /* PSYLLID_NODE_BINDING_HH_ */
