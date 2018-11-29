################
# applications #
################

if( Psyllid_ENABLE_EXECUTABLES )

    get_property( all_libs GLOBAL PROPERTY ${PROJECT_NAME}_LIBRARIES )
    message( STATUS ">>>>> Psyllid executables library dependencies: ${all_libs}" )
    message( STATUS "      midge_LIBRARIES: ${Midge_EXE_LIBRARIES}" )

    set( lib_dependencies
        PsyllidUtility
        PsyllidData
        #PsyllidDAQ
        PsyllidControl
    )

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

    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
