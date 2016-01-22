###########
# control #
###########

set( headers
    daq_control.hh
    node_manager.hh
    request_receiver.hh
)

set( sources
    daq_control.cc
    node_manager.cc
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


