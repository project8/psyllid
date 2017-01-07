###########
# control #
###########

set( headers
    control_access.hh
    daq_control.hh
    node_builder.hh
    node_config_preset.hh
    node_manager.hh
    persistent_store.hh
    request_receiver.hh
    run_server.hh
    server_config.hh
    stream_manager.hh
)

set( sources
    control_access.cc
    daq_control.cc
    node_builder.cc
    node_config_preset.cc
    node_manager.cc
    persistent_store.cc
    request_receiver.cc
    run_server.cc
    server_config.cc
    stream_manager.cc
)

set( dependencies
    PsyllidUtility
)

###########
# library #
###########

midge_library( PsyllidControl sources dependencies )
pbuilder_install_headers( ${headers} )


