###########
# control #
###########

set( headers
    daq_control.hh
    daq_worker.hh
    node_config_preset.hh
    node_manager.hh
    persistent_store.hh
    request_receiver.hh
    run_server.hh
)

set( sources
    daq_control.cc
    daq_worker.cc
    node_config_preset.cc
    node_manager.cc
    persistent_store.cc
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


