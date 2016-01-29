########
# data #
########

set( headers
    #egg_writer.hh
    #event_builder.hh
    #single_value_trigger.hh
    udp_receiver.hh
)

set( sources
    #egg_writer.cc
    #event_builder.cc
    #single_value_trigger.cc
    udp_receiver.cc
)

set( dependencies
    PsyllidUtility
    PsyllidData
)

###########
# library #
###########

midge_library( PsyllidNodes sources dependencies )
pbuilder_install_headers( ${headers} )
