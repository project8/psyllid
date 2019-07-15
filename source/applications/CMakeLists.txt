################
# applications #
################

if( Psyllid_ENABLE_EXECUTABLES )

    set( lib_dependencies
        PsyllidUtility
        PsyllidData
        PsyllidDAQ
        PsyllidControl
    )

    set( programs
        grab_packet
        psyllid
        roach_daq_streaming_1chan
    )

    if( Psyllid_BUILD_FPA )
        set( programs
             ${programs}
             roach_daq_1chan
        )
    endif( Psyllid_BUILD_FPA )

    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
