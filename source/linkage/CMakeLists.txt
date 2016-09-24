###########
# linkage #
###########

set( headers
    packet_buffer.hh
    udp_server.hh
    udp_server_socket.hh
)
set( sources
	packet_buffer.cc
    udp_server.cc
    udp_server_socket.cc
)

if( UNIX AND NOT APPLE )
    set( headers
        ${headers}
        fast_packet_acq.hh
        fpa_factory.hh
        packet_distributor.hh
        packet_eater.hh
        udp_server_fpa.hh
    )
    set( sources
        ${sources}
        fast_packet_acq.cc
        fpa_factory.cc
        packet_distributor.cc
        packet_eater.cc
        udp_server_fpa.cc
    )
endif( UNIX AND NOT APPLE )

set( dependencies
    PsyllidUtility
)


###########
# library #
###########

midge_library( PsyllidLinkage sources dependencies )
pbuilder_install_headers( ${headers} )

