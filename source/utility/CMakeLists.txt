###########
# utility #
###########

set( headers
    byte_swap.hh
    psyllid_error.hh
    psyllid_version.hh
)
set( sources
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

pbuilder_library(
    TARGET PsyllidUtility
    SOURCES ${sources}
)

pbuilder_install_headers( ${headers} )
