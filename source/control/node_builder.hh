/*
 * node_builder.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_NODE_BUILDER_HH_
#define PSYLLID_NODE_BUILDER_HH_

#include "control_access.hh"

#include "psyllid_error.hh"

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

            void configure( const scarab::param_node& a_config );
            void replace_config( const scarab::param_node& a_config );

            void dump_config( scarab::param_node& a_config );

            virtual midge::node* build() = 0;

        public:
            virtual void apply_config( midge::node* a_node ) = 0;
            virtual void extract_config( const midge::node* a_node ) = 0;

            /// throws psyllid::error if the command is unknown or fails
            virtual void run_command( midge::node* a_node, const scarab::param_node& a_config ) = 0;

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

            /// Builds a new node and applies the builder's configuration information
            virtual midge::node* build();

        public:
            /// Applies the builder's configuration information to the given node
            /// Throws psyllid::error if the node is of the wrong type or if applying the configuration fails
            virtual void apply_config( midge::node* a_node );
            /// Dumps the configuration from the given node, and replaces the builder's configuration with it
            /// Throws psyllid::error if the node is of the wrong type or if the extraction fails
            virtual void extract_config( const midge::node* a_node );
            /// Dumps the configuration from the given node and returns it to the caller; does not affect the builder's configuration information
            /// Throws psyllid::error if the node is the wrong type or if the extraction fails
            virtual void extract_config( const midge::node* a_node, scarab::param_node& a_config );

            /// Calls a command on the given node
            /// Throws psyllid::error if the command is unknown or fails
            virtual void run_command( midge::node* a_node, const scarab::param_node& a_config );


        private:
            virtual void apply_config( x_node_type* a_node, const scarab::param_node& a_config ) = 0;
            virtual void dump_config( const x_node_type* a_node, scarab::param_node& a_config ) = 0;

            /// in derived classes, should throw a std::exception if the command fails, and return false if the command is unrecognized
            virtual bool do_run_command( x_node_type* a_node, const scarab::param_node& a_cmd );
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

        // before we do anything else, get the default configuration and merge anything in f_config with it
        scarab::param_node t_temp_config( f_config );
        f_config.clear();
        dump_config( t_node, f_config );
        f_config.merge( t_temp_config );

        control_access* t_cont_acc = dynamic_cast< control_access* >( t_node );
        if( t_cont_acc != nullptr )
        {
            t_cont_acc->set_daq_control( f_daq_control );
        }

        apply_config( t_node, f_config );
        t_node->set_name( f_name );

        return t_node;
    }

    template< class x_node_type >
    void _node_builder< x_node_type >::apply_config( midge::node* a_node )
    {
        x_node_type* t_derived_node = dynamic_cast< x_node_type* >( a_node );
        if( t_derived_node == nullptr )
        {
            throw error() << "Node type does not match builder type (apply_config(node*))";
        }
        try
        {
            apply_config( t_derived_node, f_config );
        }
        catch( std::exception& e )
        {
            throw psyllid::error() << e.what();
        }
        return;
    }

    template< class x_node_type >
    void _node_builder< x_node_type >::extract_config( const midge::node* a_node )
    {
        const x_node_type* t_derived_node = dynamic_cast< const x_node_type* >( a_node );
        if( t_derived_node == nullptr )
        {
            throw error() << "Node type does not match builder type (extract_config(node*))";
        }
        try
        {
            dump_config( t_derived_node, f_config );
        }
        catch( std::exception& e )
        {
            throw psyllid::error() << e.what();
        }
        return;
    }

    template< class x_node_type >
    void _node_builder< x_node_type >::extract_config( const midge::node* a_node, scarab::param_node& a_config )
    {
        const x_node_type* t_derived_node = dynamic_cast< const x_node_type* >( a_node );
        if( t_derived_node == nullptr )
        {
            throw error() << "Node type does not match builder type (extract_config(node*, param_node&))";
        }
        try
        {
            dump_config( t_derived_node, a_config );
        }
        catch( std::exception& e )
        {
            throw psyllid::error() << e.what();
        }
        return;
    }

    template< class x_node_type >
    void _node_builder< x_node_type >::run_command( midge::node* a_node, const scarab::param_node& a_cmd )
    {
            x_node_type* t_derived_node = dynamic_cast< x_node_type* >( a_node );
            if( t_derived_node == nullptr )
            {
                throw error() << "Node type does not match builder type (extract_config(node*, param_node&))";
            }
            try
            {
                do_run_command( t_derived_node, a_cmd );
            }
            catch( std::exception& e )
            {
                throw psyllid::error() << e.what();
            }
            return;
    }

    template< class x_node_type >
    bool _node_builder< x_node_type >::do_run_command( x_node_type*, const scarab::param_node& )
    {
        return false;
    }

#define REGISTER_NODE_AND_BUILDER( node_class, node_name ) \
    static ::scarab::registrar< ::midge::node, node_class > s_node_##node_class##_registrar( node_name ); \
    static ::scarab::registrar< ::psyllid::node_builder, node_class##_builder > s_node_builder_##node_class##_registrar( node_name );


} /* namespace psyllid */

#endif /* PSYLLID_NODE_BUILDER_HH_ */
