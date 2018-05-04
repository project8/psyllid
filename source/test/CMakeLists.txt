########
# test #
########

if( Psyllid_ENABLE_TESTING )

    set( lib_dependencies
        PsyllidUtility
        PsyllidData
        PsyllidDAQ
    )

    set( programs
        #test_event_builder
        #test_monarch3_write
        #test_server
        test_tf_roach_monitor
        test_tf_roach_receiver
    )

    if( UNIX AND NOT APPLE )
        set( programs
            ${programs}
            #test_fast_packet_acq
            test_tpacket_v3
        )
    endif( UNIX AND NOT APPLE )


    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_TESTING )
