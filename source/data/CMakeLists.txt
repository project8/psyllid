########
# data #
########

set( headers
    freq_data.hh
    id_range_event.hh
    memory_block.hh
    roach_packet.hh
    time_data.hh
    trigger_flag.hh
    iq_time_data.hh
    roach_time_data.hh
    generic_time_data.hh
    KTVarTypePhysicalArray.hh
    KTAxisProperties.hh
    KTAxisProperties_GetNBins.hh
    KTConstants.hh
)

set( sources
    freq_data.cc
    id_range_event.cc
    memory_block.cc
    roach_packet.cc
    time_data.cc
    trigger_flag.cc
    iq_time_data.cc
    roach_time_data.cc
    generic_time_data.cc
    KTVarTypePhysicalArray.cc
    KTAxisProperties.cc
)

set( dependencies
    PsyllidUtility
)

###########
# library #
###########

pbuilder_library(
    TARGET PsyllidData
    SOURCES ${sources}
    PROJECT_LIBRARIES ${dependencies}
)

pbuilder_install_headers( ${headers} )
