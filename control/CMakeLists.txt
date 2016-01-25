###########
# control #
###########

set( headers
    daq_control.hh
    node_manager.hh
    request_receiver.hh
    run_server.hh
)

set( sources
    daq_control.cc
    node_manager.cc
    request_receiver.cc
    run_server.cc
)

set( dependencies
    PsyllidUtility
)

###########
# library #
###########

midge_library( PsyllidControl sources dependencies )
pbuilder_install_headers( ${headers} )


