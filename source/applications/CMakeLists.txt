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
        #single_value_daq
        roach_daq_1chan
        psyllid
    )
    
    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
