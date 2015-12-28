########
# data #
########

set( headers
    single_value_trigger.hh
    udp_receiver.hh
)

set( sources
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
target_link_libraries( _psyllid_nodes ${dependencies} ${midge_library} ${midge_external_libraries} )

pbuilder_install_headers( ${headers} )

pbuilder_install_libraries( _psyllid_nodes )


