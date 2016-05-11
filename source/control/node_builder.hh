/*
 * node_builder.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_BUILDER_HH_
#define PSYLLID_NODE_BUILDER_HH_

#include "control_access.hh"

#include "member_variables.hh"
#include "param.hh"

namespace midge
{
    class node;
}

namespace psyllid
{

    class node_builder : public control_access
    {
        public:
            node_builder();
            virtual ~node_builder();

            void configure( const scarab::param_node& a_node );
            void replace_config( const scarab::param_node& a_node );

            void dump_config( scarab::param_node& a_node );

            virtual midge::node* build() = 0;

            mv_referrable( std::string, name );

        protected:
            scarab::param_node f_config;
    };

    template< class x_node_type >
    class _node_builder : public node_builder
    {
        public:
            _node_builder();
            virtual ~_node_builder();

            virtual midge::node* build();

        private:
            virtual void apply_config( x_node_type* a_node, const scarab::param_node& a_config ) = 0;

    };

    template< class x_node_type >
    _node_builder< x_node_type >::_node_builder()
    {}

    template< class x_node_type >
    _node_builder< x_node_type >::~_node_builder()
    {}

    template< class x_node_type >
    midge::node* _node_builder< x_node_type >::build()
    {
        x_node_type* t_node = new x_node_type();

        control_access* t_cont_acc = dynamic_cast< control_access* >( t_node );
        if( t_cont_acc != nullptr )
        {
            t_cont_acc->set_daq_control( f_daq_control );
        }

        apply_config( t_node, f_config );
        t_node->set_name( f_name );

        return t_node;
    }


#define REGISTER_NODE_AND_BUILDER( node_class, node_name ) \
    static ::scarab::registrar< ::midge::node, node_class > s_node_##node_class##_registrar( node_name ); \
    static ::scarab::registrar< ::psyllid::node_builder, node_class##_builder > s_node_builder_##node_class##_registrar( node_name );


} /* namespace psyllid */

#endif /* PSYLLID_NODE_BUILDER_HH_ */
