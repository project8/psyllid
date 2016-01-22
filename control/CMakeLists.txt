###########
# control #
###########

set( headers
    request_receiver.hh
)

set( sources
    request_receiver.cc
)

set( dependencies
    PsyllidUtility
)

###########
# library #
###########

midge_library( PsyllidControl sources dependencies )
pbuilder_install_headers( ${headers} )


