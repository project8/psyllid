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
    )

    if( Psyllid_ENABLE_FPA )
        set( programs
             ${programs}
             roach_daq_1chan
             roach_daq_streaming_1chan
        )
    endif( Psyllid_ENABLE_FPA )

    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
