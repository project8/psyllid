###########
# utility #
###########

set( headers
    byte_swap.hh
    locked_resource.hh
    message_relayer.hh
    psyllid_constants.hh
    psyllid_error.hh
    psyllid_version.hh
)
set( sources
    message_relayer.cc
    psyllid_error.cc
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

