/*
 * butterfly_house.cc
 *
 *  Created on: Feb 11, 2016
 *      Author: nsoblath
 */

#include "butterfly_house.hh"

#include "daq_control.hh"
#include "egg_writer.hh"

#include "logger.hh"
#include "param.hh"
#include "time.hh"

#include "psyllid_error.hh"

namespace psyllid
{
    LOGGER( plog, "butterfly_house" );


    butterfly_house::butterfly_house() :
            control_access(),
            f_max_file_size_mb( 500 ),
            f_file_infos(),
            f_mw_ptrs(),
            f_writers(),
            f_butterflies(),
            f_house_mutex()
    {
        LDEBUG( plog, "Butterfly house has been built" );
    }

    butterfly_house::~butterfly_house()
    {
    }


    void butterfly_house::prepare_files( const scarab::param_node* a_daq_config )
    {
        // default is 1 file
        f_file_infos.clear();

        if( a_daq_config == nullptr )
        {
            f_file_infos.resize( 1 );
        }
        else
        {
            f_file_infos.resize( a_daq_config->get_value( "n-files", 1U ) );
            set_max_file_size_mb( a_daq_config->get_value( "max-file-size-mb", get_max_file_size_mb() ) );
        }

        for( file_infos_it fi_it = f_file_infos.begin(); fi_it != f_file_infos.end(); ++fi_it )
        {
            // creating default filenames; these can be changed when the run is started
            std::stringstream t_filename_sstr;
            unsigned t_file_num = fi_it - f_file_infos.begin();
            t_filename_sstr << "psyllid_out_" << t_file_num << ".egg";
            fi_it->f_filename = t_filename_sstr.str();
            fi_it->f_description = "";
            LINFO( plog, "Prepared file <" << t_file_num << ">; default filename is <" << fi_it->f_filename << ">" );
        }
        return;
    }

    void butterfly_house::start_files()
    {
        unsigned t_run_duration = 0;
        if( ! f_daq_control.expired() )
        {
            std::shared_ptr< daq_control > t_daq_control = f_daq_control.lock();
            //f_filename = t_daq_control->run_filename();
            //f_description = t_daq_control->run_description();
            t_run_duration = t_daq_control->get_run_duration();
            //LDEBUG( plog, "Updated filename, description, and duration from daq_control" );
        }
        else
        {
            LERROR( plog, "Unable to get access to the DAQ control" );
            throw error() << "Butterfly house is unable to get access to the DAQ control";
        }

        f_mw_ptrs.clear();
        f_mw_ptrs.resize( f_file_infos.size() );
        for( unsigned t_file_num = 0; t_file_num < f_file_infos.size(); ++t_file_num )
        {
            // global setup

            f_mw_ptrs[ t_file_num ] = declare_file( f_file_infos[ t_file_num ].f_filename );
            f_mw_ptrs[ t_file_num ]->set_max_file_size( f_max_file_size_mb );

            header_wrap_ptr t_hwrap_ptr = f_mw_ptrs[ t_file_num ]->get_header();
            t_hwrap_ptr->header().SetDescription( f_file_infos[ t_file_num ].f_description );

            time_t t_raw_time = time( nullptr );
            struct tm* t_processed_time = gmtime( &t_raw_time );
            char t_timestamp[ 512 ];
            strftime( t_timestamp, 512, scarab::date_time_format, t_processed_time );
            //LWARN( plog, "raw: " << t_raw_time << "   proc'd: " << t_processed_time->tm_hour << " " << t_processed_time->tm_min << " " << t_processed_time->tm_year << "   timestamp: " << t_timestamp );
            t_hwrap_ptr->header().SetTimestamp( t_timestamp );

            t_hwrap_ptr->header().SetRunDuration( t_run_duration );
            t_hwrap_ptr->global_setup_done( true );

            // writer/stream setup

            for( auto it_writer = f_writers.begin(); it_writer != f_writers.end(); ++it_writer )
            {
                if( it_writer->second == t_file_num )
                {
                    it_writer->first->prepare_to_write( f_mw_ptrs[ t_file_num ], t_hwrap_ptr );
                }
            }
        }
    }

    void butterfly_house::finish_files()
    {
        for( auto file_it = f_mw_ptrs.begin(); file_it != f_mw_ptrs.end(); ++file_it )
        {
            (*file_it)->finish_file();
            file_it->reset();
        }
    }

    void butterfly_house::register_writer( egg_writer* a_writer, unsigned a_file_num )
    {
        //TODO: what happens if the writer has already been registered for this file number
        f_writers.insert( std::pair< egg_writer*, unsigned >( a_writer, a_file_num ) );
        return;
    }

    void butterfly_house::unregister_writer( egg_writer* a_writer )
    {
        //TODO: can this be done w/out iterating?
        //TODO: does erasing one destroy the other iterators?
        auto t_range = f_writers.equal_range( a_writer );
        for( auto it_writer = t_range.first; it_writer != t_range.second; ++it_writer )
        {
            f_writers.erase( it_writer );
        }
        return;
    }



    monarch_wrap_ptr butterfly_house::declare_file( const std::string& a_filename )
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
        try
        {
            auto t_mwp_it = f_butterflies.find( a_filename );
            if( t_mwp_it == f_butterflies.end() )
            {
                monarch_wrap_ptr t_mwp( new monarch_wrapper( a_filename ) );
                LINFO( plog, "Created egg3 file <" << a_filename << ">" );
                f_butterflies.insert( bf_value_t( a_filename, t_mwp ) );
                return t_mwp;
            }
            return t_mwp_it->second;
        }
        catch( error& e )
        {
            throw;
        }
    }
/*
    header_wrap_ptr butterfly_house::get_header( const std::string& a_filename ) const
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
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
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
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
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
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
*/

    void butterfly_house::remove_file( const std::string& a_filename )
    {
        std::unique_lock< std::mutex > t_lock( f_house_mutex );
        auto t_mwp_it = f_butterflies.find( a_filename );
        if( t_mwp_it != f_butterflies.end() )
        {
            f_butterflies.erase( t_mwp_it );
        }
        return;
    }

} /* namespace psyllid */
