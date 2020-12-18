################
# applications #
################

set( lib_dependencies
    PsyllidUtility
    PsyllidData
    PsyllidDAQ
    PsyllidControl
)

set( psyllid_exe_PROGRAMS )


# Non-psyllid executables
set( programs
    grab_packet
)

pbuilder_executables( 
    SOURCES ${programs}
    TARGETS_VAR psyllid_exe_PROGRAMS
    PROJECT_LIBRARIES ${lib_dependencies}
    PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
    PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)


# Psyllid executable
build_sandfly_executable(
    ALT_NAME psyllid
    ALT_SOURCES psyllid.cc
    SANDFLY_SUBMODULE_NAME Sandfly
    PROJECT_LIBRARIES ${lib_dependencies}
)

list( APPEND psyllid_exe_PROGRAMS psyllid )


# Export
pbuilder_component_install_and_export( 
    COMPONENT Executables
    EXETARGETS ${psyllid_exe_PROGRAMS}
)
