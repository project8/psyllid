###########
# utility #
###########

set( headers
    connection.hh
    psyllidmsg.hh
    server.hh
)
set( sources
    connection.cc
    psyllidmsg.cc
    server.cc
)



###########
# library #
###########

message( STATUS "midge library from psyllid: ${midge_library}" )

add_library( _psyllid_utility ${sources} )
target_link_libraries( _psyllid_utility ${midge_library} ${midge_external_libraries} )

pbuilder_install_headers( ${headers} )

pbuilder_install_libraries( _psyllid_utility )


