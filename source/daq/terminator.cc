/*
 * terminator.cc
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#include "terminator.hh"

#include "freq_data.hh"

#include "logger.hh"



namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( terminator_time_data, "term-time-data", terminator_time_data_binding );
    REGISTER_NODE_AND_BUILDER( terminator_freq_data, "term-freq-data", terminator_freq_data_binding );
    REGISTER_NODE_AND_BUILDER( terminator_trigger_flag, "term-trig-flag", terminator_trigger_flag_binding );

    LOGGER( plog, "terminator" );


    //************************
    // terminator_time_data
    //************************

    terminator_time_data::terminator_time_data()
    {
    }

    terminator_time_data::~terminator_time_data()
    {
    }

    void terminator_time_data::initialize()
    {
        return;
    }

    void terminator_time_data::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_command = midge::stream::s_none;

            while( ! is_canceled() )
            {
                t_command = in_stream< 0 >().get();
                if( t_command == midge::stream::s_none ) continue;
                if( t_command == midge::stream::s_error ) break;

                if( t_command == midge::stream::s_exit )
                {
                    LDEBUG( plog, "Terminator is exiting" );
                    break;
                }

                if( t_command == midge::stream::s_stop )
                {
                    LDEBUG( plog, "Terminator is stopping" );
                    continue;
                }

                if( t_command == midge::stream::s_start )
                {
                    LDEBUG( plog, "Terminator is starting" );
                    continue;
                }

                if( t_command == midge::stream::s_run )
                {
                    continue;
                }

            }

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void terminator_time_data::finalize()
    {
        return;
    }


    terminator_time_data_binding::terminator_time_data_binding() :
            _node_binding< terminator_time_data, terminator_time_data_binding >()
    {}

    terminator_time_data_binding::~terminator_time_data_binding()
    {}

    void terminator_time_data_binding::do_apply_config( terminator_time_data*, const scarab::param_node& ) const
    {}

    void terminator_time_data_binding::do_dump_config( const terminator_time_data*, scarab::param_node& ) const
    {}


    //************************
    // terminator_freq_data
    //************************

    terminator_freq_data::terminator_freq_data()
    {
    }

    terminator_freq_data::~terminator_freq_data()
    {
    }

    void terminator_freq_data::initialize()
    {
        return;
    }

    void terminator_freq_data::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_command = midge::stream::s_none;

            while( ! is_canceled() )
            {
                t_command = in_stream< 0 >().get();
                if( t_command == midge::stream::s_none ) continue;
                if( t_command == midge::stream::s_error ) break;

                if( t_command == midge::stream::s_exit )
                {
                    LDEBUG( plog, "Terminator is exiting" );
                    break;
                }

                if( t_command == midge::stream::s_stop )
                {
                    LDEBUG( plog, "Terminator is stopping" );
                    continue;
                }

                if( t_command == midge::stream::s_start )
                {
                    LDEBUG( plog, "Terminator is starting" );
                    continue;
                }

                if( t_command == midge::stream::s_run )
                {
                    continue;
                }

            }

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    void terminator_freq_data::finalize()
    {
        return;
    }


    terminator_freq_data_binding::terminator_freq_data_binding() :
            _node_binding< terminator_freq_data, terminator_freq_data_binding >()
    {}

    terminator_freq_data_binding::~terminator_freq_data_binding()
    {}

    void terminator_freq_data_binding::do_apply_config( terminator_freq_data*, const scarab::param_node& ) const
    {}

    void terminator_freq_data_binding::do_dump_config( const terminator_freq_data*, scarab::param_node& ) const
    {}


    IMPLEMENT_TERMINATOR (trigger_flag);
    /*
    terminator_trigger_flag::terminator_trigger_flag()
    {
    }

    terminator_trigger_flag::~terminator_trigger_flag()
    {
    }

    void terminator_trigger_flag::execute( midge::diptera* a_midge )
    {
        try
        {
            midge::enum_t t_command = midge::stream::s_none;

            while( ! is_canceled() )
            {
                t_command = in_stream< 0 >().get();
                if( t_command == midge::stream::s_none )
                {
                    LTRACE( plog, "Terminator s_none" );
                    continue;
                }
                if( t_command == midge::stream::s_error )
                {
                    LTRACE( plog, "Terminator s_error" );
                    break;

                if( t_command == midge::stream::s_exit )
                {
                    LDEBUG( plog, "Terminator is exiting" );
                    break;
                }

                if( t_command == midge::stream::s_stop )
                {
                    LDEBUG( plog, "Terminator is stopping" );
                    continue;
                }

                if( t_command == midge::stream::s_start )
                {
                    LDEBUG( plog, "Terminator is starting" );
                    continue;
                }

                if( t_command == midge::stream::s_run )
                {
                    LTRACE( plog, "Terminator run" );
                    continue;
                }

            }

            return;
        }
        catch(...)
        {
            if( a_midge ) a_midge->throw_ex( std::current_exception() );
            else throw;
        }
    }

    terminator_trigger_flag_binding::terminator_trigger_flag_binding() :
            _node_binding< terminator_trigger_flag, terminator_trigger_flag_binding >()
    {}

    terminator_trigger_flag_binding::~terminator_trigger_flag_binding()
    {}
    */

} /* namespace psyllid */
