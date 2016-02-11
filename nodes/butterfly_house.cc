/*
 * butterfly_house.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 */

#include "butterfly_house.hh"

#include "psyllid_error.hh"

namespace psyllid
{

    uint32_t to_uint( monarch_stage a_stage )
    {
        return static_cast< uint32_t >( a_stage );
    }
    monarch_stage to_op_t( uint32_t a_stage_uint )
    {
        return static_cast< monarch_stage >( a_stage_uint );
    }
    std::ostream& operator<<( std::ostream& a_os, monarch_stage a_stage )
    {
        return a_os << to_uint( a_stage );
    }


    header_wrapper::header_wrapper( monarch3::Monarch3& a_monarch ) :
        f_header( a_monarch.GetHeader() )
    {
        if( ! f_header )
        {
            throw error() << "Unable to get monarch header";
        }
    }

    header_wrapper::~header_wrapper()
    {}

    monarch3::M3Header& header_wrapper::header()
    {
        if( ! f_header ) throw error() << "Unable to write to header; the owning Monarch object must have moved beyond the preparation stage";
        return *f_header;
    }

    void header_wrapper::monarch_stage_change( monarch_stage a_new_stage )
    {
        if( a_new_stage != monarch_stage::preparing ) f_header = nullptr;
        return;
    }




    stream_wrapper::stream_wrapper( monarch3::Monarch3& a_monarch, unsigned a_stream_no ) :
        f_stream( a_monarch.GetStream( a_stream_no ) )
    {
        if( f_stream == nullptr )
        {
            throw error() << "Invalid stream number requested: " << a_stream_no;
        }
    }

    stream_wrapper::~stream_wrapper()
    {}

    monarch3::M3Stream& stream_wrapper::stream()
    {
        if( ! f_stream ) throw error() << "Unable to provide the stream; the owning Monarch object must have moved beyond the writing stage";
        return *f_stream;
    }

    void stream_wrapper::monarch_stage_change( monarch_stage a_new_stage )
    {
        if( a_new_stage != monarch_stage::writing ) f_stream = nullptr;
        return;
    }




    butterfly_house::monarch_wrapper::monarch_wrapper( const std::string& a_filename ) :
        f_monarch(),
        f_header_wrap(),
        f_stream_wraps(),
        f_stage( monarch_stage::initialized )
    {
        try
        {
            f_monarch.reset( monarch3::Monarch3::OpenForWriting( a_filename ) );
        }
        catch( monarch3::M3Exception& e )
        {
            throw error() << "Unable to open the file <" << a_filename << "\n" <<
                    "Reason: " << e.what();
        }
    }

    butterfly_house::monarch_wrapper::~monarch_wrapper()
    {
        if( f_monarch ) f_monarch->FinishWriting();
    }

    header_wrap_ptr butterfly_house::monarch_wrapper::get_header()
    {
        if( f_stage == monarch_stage::initialized )
        {
            try
            {
                header_wrap_ptr t_header_ptr( new header_wrapper( *f_monarch.get() ) );
                f_header_wrap = t_header_ptr;
            }
            catch( error& e )
            {
                throw;
            }
            set_stage( monarch_stage::preparing );
        }
        if( f_stage != monarch_stage::preparing ) throw error() << "Invalid monarch stage for getting the header: " << f_stage;

        return f_header_wrap;
    }

    stream_wrap_ptr butterfly_house::monarch_wrapper::get_stream( unsigned a_stream_no )
    {
        if( f_stage == monarch_stage::preparing )
        {
            f_monarch->WriteHeader();
            set_stage( monarch_stage::writing );
        }
        if( f_stage != monarch_stage::writing ) throw error() << "Invalid monarch stage for getting a stream: " << f_stage;

        if( a_stream_no >= f_stream_wraps.size() )
        {
            f_stream_wraps.resize( a_stream_no + 1 );
        }

        if( ! f_stream_wraps[ a_stream_no ] )
        {
            try
            {
                stream_wrap_ptr t_stream_ptr( new stream_wrapper( *f_monarch.get(), a_stream_no ) );
                f_stream_wraps[ a_stream_no ] = t_stream_ptr;
            }
            catch( error& e )
            {
                throw;
            }
        }
        return f_stream_wraps[ a_stream_no ];
    }

    void butterfly_house::monarch_wrapper::finish_file()
    {
        f_monarch->FinishWriting();
        set_stage( monarch_stage::finished );
        f_monarch.reset();
    }

    void butterfly_house::monarch_wrapper::set_stage( monarch_stage a_stage )
    {
        f_stage = a_stage;
        if( f_header_wrap ) f_header_wrap->monarch_stage_change( a_stage );
        for( std::vector< stream_wrap_ptr >::iterator t_it = f_stream_wraps.begin(); t_it != f_stream_wraps.end(); ++t_it )
        {
            if( *t_it )
            {
                (*t_it)->monarch_stage_change( a_stage );
            }
        }
    }







    butterfly_house::butterfly_house() :
            f_butterflies()
    {
    }

    butterfly_house::~butterfly_house()
    {
    }

    void butterfly_house::declare_file( const std::string& a_filename )
    {
        try
        {
            if( f_butterflies.find( a_filename ) != f_butterflies.end() )
            {
                f_butterflies.insert( bf_value_t( a_filename, monarch_wrap_ptr( new monarch_wrapper( a_filename ) ) ) );
            }
            return;
        }
        catch( error& e )
        {
            throw;
        }
    }

    header_wrap_ptr butterfly_house::get_header( const std::string& a_filename ) const
    {
        try
        {
            bf_cit_t t_it = f_butterflies.find( a_filename );
            if( t_it == f_butterflies.end() )
            {
                throw error() << "No Monarch present with filename <" << a_filename << ">";
            }
            return t_it->second->get_header();
        }
        catch( error& e )
        {
            throw;
        }
    }

    stream_wrap_ptr butterfly_house::get_stream( const std::string& a_filename, unsigned a_stream_no ) const
    {
        try
        {
            bf_cit_t t_it = f_butterflies.find( a_filename );
            if( t_it == f_butterflies.end() )
            {
                throw error() << "No Monarch present with filename <" << a_filename << ">";
            }
            return t_it->second->get_stream( a_stream_no );
        }
        catch( error& e )
        {
            throw;
        }
    }

    void butterfly_house::write_file( const std::string& a_filename )
    {
        try
        {
            bf_it_t t_it = f_butterflies.find( a_filename );
            if( t_it == f_butterflies.end() )
            {
                throw error() << "No Monarch present with filename <" << a_filename << ">";
            }
            t_it->second->finish_file();
            return;
        }
        catch( error& e )
        {
            throw;
        }
    }


} /* namespace psyllid */
