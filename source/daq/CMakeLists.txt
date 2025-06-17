#######
# daq #
#######

set( headers
    data_producer.hh
    egg_writer.hh
    egg3_reader.hh
    event_builder.hh
    #single_value_trigger.hh
    frequency_mask_trigger.hh
    locust_egg_reader.hh
    packet_receiver_socket.hh
    roach_config.hh
    streaming_writer.hh
    terminator.hh
    tf_roach_monitor.hh
    tf_roach_receiver.hh
    triggered_writer.hh
)

set( sources
    data_producer.cc
    egg_writer.cc
    egg3_reader.cc
    event_builder.cc
    #single_value_trigger.cc
    frequency_mask_trigger.cc
    locust_egg_reader.cc
    packet_receiver_socket.cc
    roach_config.cc
    streaming_writer.cc
    terminator.cc
    tf_roach_monitor.cc
    tf_roach_receiver.cc
    triggered_writer.cc
)

if( Psyllid_ENABLE_FFTW )
    set( headers
        ${headers}
        frequency_transform.hh
    )
    set( sources
        ${sources}
        frequency_transform.cc
    )
endif( Psyllid_ENABLE_FFTW )

if( Psyllid_ENABLE_STREAMED_FREQUENCY_OUTPUT )
    set( headers 
        ${sources} 
        streaming_frequency_writer.hh 
    )

    set( sources 
        ${sources} 
        streaming_frequency_writer.cc 
    )
endif( Psyllid_ENABLE_STREAMED_FREQUENCY_OUTPUT )

if( Psyllid_BUILD_FPA )
    set( headers
        ${headers}
        packet_receiver_fpa.hh
    )

    set( sources
        ${sources}
        packet_receiver_fpa.cc
    )
endif( Psyllid_BUILD_FPA )

set( dependencies
    PsyllidControl
    PsyllidData
    PsyllidUtility
)

###########
# library #
###########

pbuilder_library(
    TARGET PsyllidDAQ
    SOURCES ${sources}
    PROJECT_LIBRARIES ${dependencies}
    PUBLIC_EXTERNAL_LIBRARIES ${PUBLIC_EXT_LIBS}
    PRIVATE_EXTERNAL_LIBRARIES ${PRIVATE_EXT_LIBS}
)

pbuilder_install_headers( ${headers} )
