################
# applications #
################

if( Psyllid_ENABLE_EXECUTABLES )

    get_property( all_libs GLOBAL PROPERTY ${PROJECT_NAME}_LIBRARIES )

    set( programs
        #grab_packet
        psyllid
    )

    if( Psyllid_BUILD_FPA )
        set( programs
             ${programs}
             #roach_daq_1chan
             #roach_daq_streaming_1chan
        )
    endif( Psyllid_BUILD_FPA )

    pbuilder_executables( programs all_libs )

endif( Psyllid_ENABLE_EXECUTABLES )
