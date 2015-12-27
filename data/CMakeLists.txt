########
# data #
########

set( headers
    roach_packet.hh
)

set( sources
    roach_packet.cc
)

set( dependencies
    _psyllid_utility
)

###########
# library #
###########

add_library( _psyllid_data ${sources} )
target_link_libraries( _psyllid_data ${dependencies} ${midge_library} ${midge_external_libraries} )

pbuilder_install_headers( ${headers} )

pbuilder_install_libraries( _psyllid_data )


