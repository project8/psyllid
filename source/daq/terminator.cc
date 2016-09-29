/*
 * terminator_freq_data.cc
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#include "terminator.hh"

#include "freq_data.hh"

#include "logger.hh"



namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( terminator_freq_data, "term-freq-data" );

    LOGGER( plog, "terminator_freq_data" );


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

    void terminator_freq_data::execute()
    {
        midge::enum_t t_command = midge::stream::s_none;

        //freq_data* t_data = nullptr;

        while( true )
        {
            t_command = in_stream< 0 >().get();

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

            //t_data = in_stream< 0 >().data();

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

    void terminator_freq_data::finalize()
    {
        return;
    }


    terminator_freq_data_builder::terminator_freq_data_builder() :
            _node_builder< terminator_freq_data >()
    {}

    terminator_freq_data_builder::~terminator_freq_data_builder()
    {}

    void terminator_freq_data_builder::apply_config( terminator_freq_data*, const scarab::param_node& )
    {}

} /* namespace psyllid */
