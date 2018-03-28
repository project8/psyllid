#######
# daq #
#######

set( headers
    egg3_reader.hh
    event_builder.hh
    #single_value_trigger.hh
    frequency_mask_trigger.hh
    packet_receiver_socket.hh
    roach_config.hh
    streaming_writer.hh
    terminator.hh
    tf_roach_monitor.hh
    tf_roach_receiver.hh
    triggered_writer.hh
)

set( sources
    egg3_reader.cc
    event_builder.cc
    #single_value_trigger.cc
    frequency_mask_trigger.cc
    packet_receiver_socket.cc
    roach_config.cc
    streaming_writer.cc
    terminator.cc
    tf_roach_monitor.cc
    tf_roach_receiver.cc
    triggered_writer.cc
)

if( UNIX AND NOT APPLE )
set( headers 
    ${headers}
    packet_receiver_fpa.hh
)

set( sources
    ${sources}
    packet_receiver_fpa.cc
)
endif( UNIX AND NOT APPLE )

set( dependencies
    PsyllidUtility
    PsyllidData
    PsyllidControl
)

###########
# library #
###########

midge_library( PsyllidDAQ sources dependencies )
pbuilder_install_headers( ${headers} )
