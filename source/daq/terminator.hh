/*
 * terminator_freq_data.hh
 *
 *  Created on: May 31, 2016
 *      Author: nsoblath
 */

#ifndef PSYLLID_TERMINATOR_FREQ_DATA_HH_
#define PSYLLID_TERMINATOR_FREQ_DATA_HH_

#include "node_builder.hh"

#include "consumer.hh"

#include "freq_data.hh"
#include "time_data.hh"
#include "trigger_flag.hh"

namespace psyllid
{

#define DEFINE_TERMINATOR( data_class ) \
        class terminator_##data_class : public midge::_consumer< terminator_##data_class, typelist_1( data_class ) > \
        { \
            public: \
                terminator_##data_class(); \
                virtual ~terminator_##data_class(); \
            public: \
                virtual void initialize() {}; \
                virtual void execute( midge::diptera* a_midge = nullptr ); \
                virtual void finalize() {}; \
        }; \
        class terminator_##data_class##_binding : public _node_binding< terminator_##data_class, terminator_##data_class##_binding > \
        { \
            public: \
                terminator_##data_class##_binding(); \
                virtual ~terminator_##data_class##_binding(); \
            private: \
                virtual void do_apply_config( terminator_##data_class*, const scarab::param_node& ) const {}; \
                virtual void do_dump_config( const terminator_##data_class*, scarab::param_node& ) const {}; \
        };

#define IMPLEMENT_TERMINATOR( data_class ) \
        terminator_##data_class::terminator_##data_class() {} \
        terminator_##data_class::~terminator_##data_class() {} \
        void terminator_##data_class::execute( midge::diptera* a_midge ) \
        { \
            try \
            { \
                midge::enum_t t_command = midge::stream::s_none; \
                while( ! is_canceled() ) \
                { \
                    t_command = in_stream< 0 >().get(); \
                    if( t_command == midge::stream::s_none ) continue; \
                    if( t_command == midge::stream::s_error ) break; \
                    if( t_command == midge::stream::s_exit ) \
                    { \
                        LDEBUG( plog, "Terminator is exiting" ); \
                        break; \
                    } \
                    if( t_command == midge::stream::s_stop ) \
                    { \
                        LDEBUG( plog, "Terminator is stopping" ); \
                        continue; \
                    } \
                    if( t_command == midge::stream::s_start ) \
                    { \
                        LDEBUG( plog, "Terminator is starting" ); \
                        continue; \
                    } \
                    if( t_command == midge::stream::s_run ) \
                    { \
                        continue; \
                    } \
                } \
                return; \
            } \
            catch(...) \
            { \
                if( a_midge ) a_midge->throw_ex( std::current_exception() ); \
                else throw; \
            } \
        } \
        terminator_##data_class##_binding::terminator_##data_class##_binding() : _node_binding< terminator_##data_class, terminator_##data_class##_binding >() {} \
        terminator_##data_class##_binding::~terminator_##data_class##_binding() {}



    class terminator_time_data :
            public midge::_consumer< terminator_time_data, typelist_1( time_data ) >
    {
        public:
            terminator_time_data();
            virtual ~terminator_time_data();

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

    };

    class terminator_time_data_binding : public _node_binding< terminator_time_data, terminator_time_data_binding >
    {
        public:
            terminator_time_data_binding();
            virtual ~terminator_time_data_binding();

        private:
            virtual void do_apply_config( terminator_time_data* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const terminator_time_data* a_node, scarab::param_node& a_config ) const;
    };


    class terminator_freq_data :
            public midge::_consumer< terminator_freq_data, typelist_1( freq_data ) >
    {
        public:
            terminator_freq_data();
            virtual ~terminator_freq_data();

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

    };

    class terminator_freq_data_binding : public _node_binding< terminator_freq_data, terminator_freq_data_binding >
    {
        public:
            terminator_freq_data_binding();
            virtual ~terminator_freq_data_binding();

        private:
            virtual void do_apply_config( terminator_freq_data* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const terminator_freq_data* a_node, scarab::param_node& a_config ) const;
    };


    DEFINE_TERMINATOR( trigger_flag );
/*
    class terminator_trig_flag_data :
            public midge::_consumer< terminator_trig_flag_data, typelist_1( trigger_flag ) >
    {
        public:
            terminator_trig_flag_data();
            virtual ~terminator_trig_flag_data();

        public:
            virtual void initialize();
            virtual void execute( midge::diptera* a_midge = nullptr );
            virtual void finalize();

    };

    class terminator_trig_flag_data_binding : public _node_binding< terminator_trig_flag_data, terminator_trig_flag_data_binding >
    {
        public:
            terminator_trig_flag_data_binding();
            virtual ~terminator_trig_flag_data_binding();

        private:
            virtual void do_apply_config( terminator_trig_flag_data* a_node, const scarab::param_node& a_config ) const;
            virtual void do_dump_config( const terminator_trig_flag_data* a_node, scarab::param_node& a_config ) const;
    };
*/

} /* namespace psyllid */

#endif /* PSYLLID_TERMINATOR_FREQ_DATA_HH_ */
