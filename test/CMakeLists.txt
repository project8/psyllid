########
# test #
########

if( Psyllid_ENABLE_TESTING )

    set( lib_dependencies
        PsyllidUtility
        PsyllidData
        PsyllidNodes
    )
    
    set( programs
        #test_event_builder
        test_monarch3_write
        #test_server
        test_udp_receiver
    )
    
    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_TESTING )
