###########
# utility #
###########

set( headers
    psyllid_error.hh
    psyllid_version.hh
    signal_handler.hh
    udp_server.hh
)
set( sources
    psyllid_error.hh
    signal_handler.cc
    udp_server.cc
)

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

