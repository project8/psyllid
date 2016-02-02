/*
 * configurator.hh
 *
 *  Created on: Nov 5, 2013
 *      Author: nsoblath
 */

#ifndef PSYLLID_CONFIGURATOR_HH_
#define PSYLLID_CONFIGURATOR_HH_

#include "param.hh"

#include "psyllid_error.hh"

namespace psyllid
{
    class configurator
    {
        public:
            configurator( int an_argc, char** an_argv, scarab::param_node* a_default = NULL );
            virtual ~configurator();

            const std::string& exe_name() const;

            scarab::param_node& config();
            const scarab::param_node& config() const;

            template< typename XReturnType >
            XReturnType get( const std::string& a_name ) const;

            template< typename XReturnType >
            XReturnType get( const std::string& a_name, XReturnType a_default ) const;

        private:
            std::string f_exe_name; // the name of the executable being used

            scarab::param_node* f_master_config;

            mutable scarab::param* f_param_buffer;

            std::string f_string_buffer;
    };

    template< typename XReturnType >
    XReturnType configurator::get( const std::string& a_name ) const
    {
        f_param_buffer = const_cast< scarab::param* >( f_master_config->at( a_name ) );
        if( f_param_buffer != NULL && f_param_buffer->is_value() )
        {
            return f_param_buffer->as_value().get< XReturnType >();
        }
        throw error() << "configurator does not have a value for <" << a_name << ">";
    }

    template< typename XReturnType >
    XReturnType configurator::get( const std::string& a_name, XReturnType a_default ) const
    {
        f_param_buffer = const_cast< scarab::param* >( f_master_config->at( a_name ) );
        if( f_param_buffer != NULL && f_param_buffer->is_value() )
        {
            return f_param_buffer->as_value().get< XReturnType >();
        }
        return a_default;

    }

    inline const std::string& configurator::exe_name() const
    {
        return f_exe_name;
    }

    inline scarab::param_node& configurator::config()
    {
        return *f_master_config;
    }

    inline const scarab::param_node& configurator::config() const
    {
        return *f_master_config;
    }


} /* namespace psyllid */
#endif /* PSYLLID_CONFIGURATOR_HH_ */
