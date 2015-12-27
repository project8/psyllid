###########
# utility #
###########

set( headers
    psyllidmsg.hh
    server.hh
)
set( sources
    psyllidmsg.cc
    server.cc
)



###########
# library #
###########

add_library( _psyllid_utility ${sources} )
target_link_libraries( _psyllid_utility ${midge_library} ${midge_external_libraries} )

pbuilder_install_headers( ${headers} )

pbuilder_install_libraries( _psyllid_utility )


