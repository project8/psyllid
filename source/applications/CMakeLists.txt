################
# applications #
################

if( Psyllid_ENABLE_EXECUTABLES )

    set( lib_dependencies 
        PsyllidUtility
        PsyllidData
        PsyllidDAQ
        PsyllidControl
    )
    
    set( programs
        grab_packet
        #single_value_daq
        roach_daq_1chan
        roach_daq_streaming_1chan
        psyllid
    )
    
    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
