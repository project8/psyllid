/*
 * mt_signal_handler.cc
 *
 *  Created on: Dec 3, 2013
 *      Author: nsoblath
 */

#include "signal_handler.hh"

#include "logger.hh"

#include "psyllid_error.hh"

#include "cancelable.hh"

#include <signal.h>
#ifndef _WIN32
#include <unistd.h> //usleep
#endif

namespace psyllid
{
    LOGGER( plog, "signal_handler" );

    bool signal_handler::f_got_exit_signal = false;

    bool signal_handler::f_handling_sig_int = false;
    bool signal_handler::f_handling_sig_quit = false;

    std::mutex signal_handler::f_mutex;
    signal_handler::cancelers signal_handler::f_cancelers;

    signal_handler::signal_handler()
    {
        if( ! f_handling_sig_int && signal( SIGINT, signal_handler::handler_cancel_threads ) == SIG_ERR )
        {
            throw error() << "Unable to handle SIGINT\n";
        }
        else
        {
            f_handling_sig_int = true;
        }

#ifndef _WIN32
        if( ! f_handling_sig_quit && signal( SIGQUIT, signal_handler::handler_cancel_threads ) == SIG_ERR )
        {
            throw error() << "Unable to handle SIGQUIT\n";
        }
        else
        {
            f_handling_sig_quit = true;
        }

        if( signal(SIGPIPE, SIG_IGN) == SIG_ERR )
        {
            throw error() << "Unable to ignore SIGPIPE\n";
        }
#endif
    }

    signal_handler::~signal_handler()
    {
    }

    void signal_handler::add_cancelable( midge::cancelable* a_cancelable )
    {
        f_mutex.lock();
        f_cancelers.insert( a_cancelable );
        f_mutex.unlock();
        return;
    }

    void signal_handler::remove_cancelable( midge::cancelable* a_cancelable )
    {
        f_mutex.lock();
        f_cancelers.erase( a_cancelable );
        f_mutex.unlock();
        return;
    }

    void signal_handler::reset()
    {
        f_mutex.lock();
        f_got_exit_signal = false;
        f_handling_sig_int = false;
        f_handling_sig_quit = false;
        f_cancelers.clear();
        f_mutex.unlock();
        return;
    }

    bool signal_handler::got_exit_signal()
    {
        return f_got_exit_signal;
    }

    void signal_handler::handler_cancel_threads( int )
    {
        print_message();

        f_mutex.lock();
        f_got_exit_signal = true;
        while( ! f_cancelers.empty() )
        {
            (*f_cancelers.begin())->cancel();
            f_cancelers.erase( f_cancelers.begin() );
#ifndef _WIN32
            usleep( 100 );
#else
            Sleep( 1 );
#endif
        }
        f_mutex.unlock();

#ifdef _WIN32
        ExitProcess( 1 );
#endif
        return;
    }

    void signal_handler::print_message()
    {
        INFO( plog, "\n\nHello!  Your signal is being handled by signal_handler.\n"
             << "Have a nice day!\n" );
        return;
    }

} /* namespace mantis */
