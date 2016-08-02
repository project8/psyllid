###########
# utility #
###########

set( headers
    byte_swap.hh
    locked_resource.hh
    psyllid_constants.hh
    psyllid_error.hh
    psyllid_version.hh
    signal_handler.hh
    udp_server.hh
)
set( sources
    psyllid_error.cc
    signal_handler.cc
    udp_server.cc
)

if( UNIX )
    set( headers
        ${headers}
        packet_eater.hh
    )
    set( sources
        ${sources}
        packet_eater.cc
    )
endif( UNIX )

configure_file( psyllid_version.cc.in ${CMAKE_CURRENT_BINARY_DIR}/psyllid_version.cc )
set( sources
    ${sources}
    ${CMAKE_CURRENT_BINARY_DIR}/psyllid_version.cc
)


###########
# library #
###########

midge_library( PsyllidUtility sources dependencies )
pbuilder_install_headers( ${headers} )

