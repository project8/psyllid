########
# data #
########

set( headers
    butterfly_house.hh
    #egg_writer.hh
    event_builder.hh
    #single_value_trigger.hh
    frequency_mask_trigger.hh
    roach_config.hh
    udp_receiver.hh
)

set( sources
    butterfly_house.cc
    #egg_writer.cc
    event_builder.cc
    #single_value_trigger.cc
    frequency_mask_trigger.cc
    roach_config.cc
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

midge_library( PsyllidNodes sources dependencies )
pbuilder_install_headers( ${headers} )
