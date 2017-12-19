/*
 * egg3_reader.cc
 *
 *  Created on: Dec. 14, 2017
 *      Author: laroque
 */

#include "egg3_reader.hh"
#include "psyllid_error.hh"

#include "logger.hh"
#include "M3Monarch.hh"
#include "param.hh"

using midge::stream;

namespace psyllid
{
    REGISTER_NODE_AND_BUILDER( egg3_reader, "egg3-reader", egg3_reader_binding );

    LOGGER( plog, "egg3_reader" );

    // egg3_reader methods
    egg3_reader::egg3_reader() :
            f_egg( nullptr ),
            f_egg_path( "/dev/null" ),
            f_length( 10 )
    {
    }

    egg3_reader::~egg3_reader()
    {
        cleanup_file();
    }

    void egg3_reader::initialize()
    {
        out_buffer< 0 >().initialize( f_length );

        LDEBUG( plog, "opening egg file [" << f_egg_path << "]" );
        f_egg = monarch3::Monarch3::OpenForReading( f_egg_path );

        // do i actually care about reading or getting the header?
        // if so, do i do it here or in execute? can i update the DAQ settings so that the header info is retained in the output?
        //t_egg_file->ReadHeader();
        //const monarch3::M3Header *t_egg_header = t_egg_file->GetHeader();
        return;

    }

    void egg3_reader::execute( midge::diptera* a_midge )
    {
        try
        {
            LDEBUG( plog, "Executing the egg3_reader" );
        }
        catch(...)
        {
            a_midge->throw_ex( std::current_exception() );
        }
    }

    void egg3_reader::finalize()
    {
        LDEBUG( plog, "finalize the egg3_reader" );
        out_buffer< 0 >().finalize();
        cleanup_file();
    }

    void egg3_reader::cleanup_file()
    {
        LDEBUG( plog, "clean egg" );
        if ( f_egg->GetState() != monarch3::Monarch3::eClosed ) {
            LDEBUG( plog, "actually close egg" );
            f_egg->FinishReading();
        }
    }

    // egg3_reader_binding methods
    egg3_reader_binding::egg3_reader_binding() :
            _node_binding< egg3_reader, egg3_reader_binding >()
    {
    }

    egg3_reader_binding::~egg3_reader_binding()
    {
    }

    void egg3_reader_binding::do_apply_config( egg3_reader* a_node, const scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Configuring egg3_reader with:\n" << a_config );
        a_node->set_egg_path( a_config.get_value( "egg_path", a_node->get_egg_path() ) );
        a_node->set_length( a_config.get_value( "length", a_node->get_length() ) );
        return;
    }

    void egg3_reader_binding::do_dump_config( const egg3_reader* a_node, scarab::param_node& a_config ) const
    {
        LDEBUG( plog, "Dumping configuration for egg3_reader" );
        a_config.add( "egg_path", new scarab::param_value( a_node->get_egg_path() ) );
        a_config.add( "length", new scarab::param_value( a_node->get_length() ) );
        return;
    }

} /* namespace psyllid */
