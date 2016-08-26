###########
# linkage #
###########

set( headers
    packet_buffer.hh
    packet_distributor.hh
    udp_server.hh
    udp_server_buffered.hh
    udp_server_socket.hh
)
set( sources
	packet_buffer.cc
	packet_distributor.cc
    udp_server.cc
    udp_server_buffered.cc
    udp_server_socket.cc
)

if( UNIX )
    set( headers
        ${headers}
        packet_eater.hh
        packet_eater_manager.hh
    )
    set( sources
        ${sources}
        packet_eater.cc
        packet_eater_manager.cc
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

