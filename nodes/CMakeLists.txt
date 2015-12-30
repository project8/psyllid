########
# data #
########

set( headers
    egg_writer.hh
    event_builder.hh
    single_value_trigger.hh
    udp_receiver.hh
)

set( sources
    egg_writer.cc
    event_builder.cc
    single_value_trigger.cc
    udp_receiver.cc
)

set( dependencies
    _psyllid_utility
    _psyllid_data
)

###########
# library #
###########

add_library( _psyllid_nodes ${sources} )
target_link_libraries( _psyllid_nodes ${dependencies} ${midge_library} ${EXTERNAL_LIBRARIES} )

pbuilder_install_headers( ${headers} )

pbuilder_install_libraries( _psyllid_nodes )


