############
# examples #
############

# example config files

set( psyllid_CONFIGS
    eb_fmt_1ch_fpa.yaml
    eb_fmt_1ch_socket.yaml
    fmt_1ch_fpa.yaml
    fmt_1ch_socket.yaml
    str_1ch_dataprod.yaml
    str_1ch_fpa.yaml
    str_1ch_socket_batch.yaml
    str_1ch_socket_custom.yaml
    str_1ch_socket.yaml
    str_3ch_fpa.yaml
)

pbuilder_install_config( ${psyllid_CONFIGS} )


# example executables

if( Psyllid_ENABLE_EXECUTABLES )

    set( lib_dependencies
        PsyllidUtility
        PsyllidData
        PsyllidDAQ
        PsyllidControl
    )

    set( psyllid_exe_PROGRAMS )


    # Non-psyllid executables
    set( programs
    )

    if( Psyllid_BUILD_FPA )
        set( programs
                ${programs}
                roach_daq_1chan
                roach_daq_streaming_1chan
        )
    endif( Psyllid_BUILD_FPA )

    pbuilder_executables( 
        SOURCES ${programs}
        TARGETS_VAR psyllid_exe_PROGRAMS
        PROJECT_LIBRARIES ${lib_dependencies}
        PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
        PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
    )


    # Export
    # There's a chance there are no programs, so we check before installing/exporting
    if( psyllid_exe_PROGRAMS )
        pbuilder_component_install_and_export( 
            COMPONENT Examples
            EXETARGETS ${psyllid_exe_PROGRAMS}
        )
    endif()

endif( Psyllid_ENABLE_EXECUTABLES )
