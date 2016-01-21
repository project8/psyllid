###########
# utility #
###########

set( headers
    psyllidmsg.hh
    server.hh
)
set( sources
    psyllidmsg.cc
    server.cc
)



###########
# library #
###########

midge_library( PsyllidUtility sources dependencies )
pbuilder_install_headers( ${headers} )

