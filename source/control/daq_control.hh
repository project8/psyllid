/*
 * daq_control.hh
 *
 *  Created on: Jan 22, 2016
 *      Author: N.S. Oblath
 */

#ifndef PSYLLID_DAQ_CONTROL_HH_
#define PSYLLID_DAQ_CONTROL_HH_

#include "run_control.hh"


namespace psyllid
{

    /*!
     @class daq_control
     @author N. S. Oblath

     @brief Adds monarch-based file control to run_control

     @details
    */
    class daq_control : public sandfly::run_control
    {

        public:
            daq_control( const scarab::param_node& a_primary_config, std::shared_ptr< sandfly::stream_manager > a_mgr );
            virtual ~daq_control();

        protected:
            /// Handle called to perform initialization
            virtual void on_initialize();
            /// Handle called before run takes place (i.e. control is unpaused)
            virtual void on_pre_run();
            /// Handle called after run finishes (i.e. control is paused)
            virtual void on_post_run();


        public:

        public:
            dripline::reply_ptr_t handle_start_run_request( const dripline::request_ptr_t a_request );

            dripline::reply_ptr_t handle_set_filename_request( const dripline::request_ptr_t a_request );
            dripline::reply_ptr_t handle_set_description_request( const dripline::request_ptr_t a_request );
            dripline::reply_ptr_t handle_set_use_monarch_request( const dripline::request_ptr_t a_request );

            dripline::reply_ptr_t handle_get_filename_request( const dripline::request_ptr_t a_request );
            dripline::reply_ptr_t handle_get_description_request( const dripline::request_ptr_t a_request );
            dripline::reply_ptr_t handle_get_use_monarch_request( const dripline::request_ptr_t a_request );


        public:
            void set_filename( const std::string& a_filename, unsigned a_file_num = 0 );
            const std::string& get_filename( unsigned a_file_num = 0 );

            void set_description( const std::string& a_desc, unsigned a_file_num = 0 );
            const std::string& get_description( unsigned a_file_num = 0 );

            mv_accessible( bool, use_monarch );

    };

} /* namespace psyllid */

#endif /* PSYLLID_DAQ_CONTROL_HH_ */
