###########
# utility #
###########

set( headers
    server.hh
)
set( sources
    server.cc
)



###########
# library #
###########

midge_library( PsyllidUtility sources dependencies )
pbuilder_install_headers( ${headers} )

