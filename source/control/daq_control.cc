/*
 * daq_control.cc
 *
 *  Created on: Jan 22, 2016
 *      Author: nsoblath
 */

#include "daq_control.hh"

#include "butterfly_house.hh"
#include "psyllid_error.hh"

#include "message_relayer.hh"


using scarab::param_array;
using scarab::param_node;
using scarab::param_value;
using scarab::param_ptr_t;

using dripline::request_ptr_t;

using std::string;

namespace psyllid
{
    LOGGER( plog, "daq_control" );

    daq_control::daq_control( const param_node& a_master_config, std::shared_ptr< sandfly::stream_manager > a_mgr ) :
            sandfly::run_control( a_master_config, a_mgr ),
            f_use_monarch( true )
    {
    }

    daq_control::~daq_control()
    {
    }

    void daq_control::on_initialize()
    {
        butterfly_house::get_instance()->prepare_files( f_daq_config );
        return;
    }

    void daq_control::on_pre_run()
    {
        if( f_use_monarch )
        {
            LDEBUG( plog, "Starting egg files" );
            try
            {
                butterfly_house::get_instance()->start_files();
            }
            catch( std::exception& e )
            {
                LERROR( plog, "Unable to start files: " << e.what() );
                f_msg_relay->slack_error( std::string("Unable to start files: ") + e.what() );
                set_status( status::error );
                LDEBUG( plog, "Canceling midge" );
                if( f_midge_pkg.have_lock() ) f_midge_pkg->cancel();
                return;
            }
        }
        return;
    }
    /// Handle called after run finishes (i.e. control is paused)
    void daq_control::on_post_run()
    {
        if( f_use_monarch )
        {
            LDEBUG( plog, "Finishing egg files" );
            try
            {
                butterfly_house::get_instance()->finish_files();
            }
            catch( std::exception& e )
            {
                LERROR( plog, "Unable to finish files: " << e.what() );
                f_msg_relay->slack_error( std::string("Unable to finish files: ") + e.what() );
                set_status( status::error );
                LDEBUG( plog, "Canceling midge" );
                if( f_midge_pkg.have_lock() ) f_midge_pkg->cancel();
                return;
            }
        }
        return;
    }

    dripline::reply_ptr_t daq_control::handle_start_run_request( const dripline::request_ptr_t a_request )
    {
        try
        {
            if( a_request->payload().is_node() )
            {
                param_node& t_payload = a_request->payload().as_node();
                if( t_payload.has( "filename" ) ) set_filename( t_payload["filename"]().as_string(), 0 );
                //TODO BUG here, if filenames exists but is not an array (only case i tried), this causes a seg fault which is not handled below
                if( t_payload.as_node().has( "filenames" ) )
                {
                    const scarab::param_array t_filenames = t_payload["filenames"].as_array();
                    for( unsigned i_fn = 0; i_fn < t_filenames.size(); ++i_fn )
                    {
                        set_filename( t_filenames[i_fn]().as_string(), i_fn );
                    }
                }

                if( t_payload.has( "description" ) ) set_description( t_payload["description"]().as_string(), 0 );
                if( t_payload.has( "descriptions" ) )
                {
                    const scarab::param_array t_descriptions = t_payload["descriptions"].as_array();
                    for( unsigned i_fn = 0; i_fn < t_descriptions.size(); ++i_fn )
                    {
                        set_description( t_descriptions[i_fn]().as_string(), i_fn );
                    }
                }

                f_run_duration = a_request->payload().get_value( "duration", f_run_duration );
            }

            start_run();
            return a_request->reply( dripline::dl_success(), "Run started" );
        }
        catch( std::exception& e )
        {
            LWARN( plog, "there was an error starting a run" );
            return a_request->reply( dripline::dl_service_error(), string( "Unable to start run: " ) + e.what() );
        }
    }


    dripline::reply_ptr_t daq_control::handle_set_filename_request( const dripline::request_ptr_t a_request )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_specifier().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_specifier().front() );
            }

            std::string t_filename =  a_request->payload()["values"][0]().as_string();
            LDEBUG( plog, "Setting filename for file <" << t_file_num << "> to <" << t_filename << ">" );
            set_filename( t_filename, t_file_num );
            return a_request->reply( dripline::dl_success(), "Filename set" );
        }
        catch( std::exception& e )
        {
            return a_request->reply( dripline::dl_service_error(), string( "Unable to set filename: " ) + e.what() );
        }
    }

    dripline::reply_ptr_t daq_control::handle_set_description_request( const dripline::request_ptr_t a_request )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_specifier().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_specifier().front() );
            }

            std::string t_description =  a_request->payload()["values"][0]().as_string();
            LDEBUG( plog, "Setting description for file <" << t_file_num << "> to <" << t_description << ">" );
            set_description( t_description, t_file_num );

            return a_request->reply( dripline::dl_success(), "Description set" );
        }
        catch( std::exception& e )
        {
            return a_request->reply( dripline::dl_service_error(), string( "Unable to set description: " ) + e.what() );
        }
    }

    dripline::reply_ptr_t daq_control::handle_set_use_monarch_request( const dripline::request_ptr_t a_request )
    {
        try
        {
            f_use_monarch =  a_request->payload()["values"][0]().as_bool();
            LDEBUG( plog, "Use-monarch set to <" << f_use_monarch << ">" );
            return a_request->reply( dripline::dl_success(), "Use Monarch set" );
        }
        catch( std::exception& e )
        {
            return a_request->reply( dripline::dl_service_error(), string( "Unable to set use-monarch: " ) + e.what() );
        }
    }

    dripline::reply_ptr_t daq_control::handle_get_filename_request( const dripline::request_ptr_t a_request )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_specifier().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_specifier().front() );
            }

            param_array t_values_array;
            t_values_array.push_back( param_value( get_filename( t_file_num ) ) );
            param_ptr_t t_payload_ptr( new param_node() );
            t_payload_ptr->as_node().add( "values", t_values_array );
            return a_request->reply( dripline::dl_success(), "Filename request completed", std::move(t_payload_ptr) );
        }
        catch( scarab::error& e )
        {
            return a_request->reply( dripline::dl_service_error(), string( "Unable to get description: " ) + e.what() );
        }
    }

    dripline::reply_ptr_t daq_control::handle_get_description_request( const dripline::request_ptr_t a_request )
    {
        try
        {
            unsigned t_file_num = 0;
            if( a_request->parsed_specifier().size() > 0)
            {
                t_file_num = std::stoi( a_request->parsed_specifier().front() );
            }

            param_array t_values_array;
            t_values_array.push_back( param_value( get_description( t_file_num ) ) );
            param_ptr_t t_payload_ptr( new param_node() );
            t_payload_ptr->as_node().add( "values", t_values_array );
            return a_request->reply( dripline::dl_success(), "Description request completed", std::move(t_payload_ptr) );
        }
        catch( scarab::error& e )
        {
            return a_request->reply( dripline::dl_service_error(), string( "Unable to get description: " ) + e.what() );
        }
    }

    dripline::reply_ptr_t daq_control::handle_get_use_monarch_request( const dripline::request_ptr_t a_request )
    {
        param_array t_values_array;
        t_values_array.push_back( param_value( f_use_monarch ) );

        param_ptr_t t_payload_ptr( new param_node() );
        t_payload_ptr->as_node().add( "values", t_values_array );

        return a_request->reply( dripline::dl_success(), "Use Monarch request completed", std::move(t_payload_ptr) );
    }


    void daq_control::set_filename( const std::string& a_filename, unsigned a_file_num )
    {
        try
        {
            butterfly_house::get_instance()->set_filename( a_filename, a_file_num );
            return;
        }
        catch( error& )
        {
            throw;
        }
    }

    const std::string& daq_control::get_filename( unsigned a_file_num )
    {
        try
        {
            return butterfly_house::get_instance()->get_filename( a_file_num );
        }
        catch( error& )
        {
            throw;
        }
    }

    void daq_control::set_description( const std::string& a_desc, unsigned a_file_num )
    {
        try
        {
            butterfly_house::get_instance()->set_description( a_desc, a_file_num );
            return;
        }
        catch( error& )
        {
            throw;
        }
    }

    const std::string& daq_control::get_description( unsigned a_file_num )
    {
        try
        {
            return butterfly_house::get_instance()->get_description( a_file_num );
        }
        catch( error& )
        {
            throw;
        }
    }

} /* namespace psyllid */
