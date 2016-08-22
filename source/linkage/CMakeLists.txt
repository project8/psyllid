###########
# linkage #
###########

set( headers
    packet_buffer.hh
    udp_server.hh
)
set( sources
	packet_buffer.cc
    udp_server.cc
)

if( UNIX )
    set( headers
        ${headers}
        packet_eater.hh
    )
    set( sources
        ${sources}
        packet_eater.cc
    )
endif( UNIX )

set( dependencies
    PsyllidUtility
)


###########
# library #
###########

midge_library( PsyllidLinkage sources dependencies )
pbuilder_install_headers( ${headers} )

