#######
# daq #
#######

set( headers
    butterfly_house.hh
    egg_writer.hh
    event_builder.hh
    #single_value_trigger.hh
    frequency_mask_trigger.hh
    monarch3_wrap.hh
    roach_config.hh
    streaming_writer.hh
    terminator.hh
    udp_receiver.hh
)

set( sources
    butterfly_house.cc
    egg_writer.cc
    event_builder.cc
    #single_value_trigger.cc
    frequency_mask_trigger.cc
    monarch3_wrap.cc
    roach_config.cc
    streaming_writer.cc
    terminator.cc
    udp_receiver.cc
)

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