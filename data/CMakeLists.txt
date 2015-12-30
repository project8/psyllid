########
# data #
########

set( headers
    freq_data.hh
    id_range_event.hh
    roach_packet.hh
    time_data.hh
    trigger_flag.hh
)

set( sources
    freq_data.cc
    id_range_event.cc
    roach_packet.cc
    time_data.cc
    trigger_flag.cc
)

set( dependencies
    _psyllid_utility
)

###########
# library #
###########

add_library( _psyllid_data ${sources} )
target_link_libraries( _psyllid_data ${dependencies} ${midge_library} ${EXTERNAL_LIBRARIES} )

pbuilder_install_headers( ${headers} )

pbuilder_install_libraries( _psyllid_data )


