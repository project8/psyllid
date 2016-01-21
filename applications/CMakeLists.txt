################
# applications #
################

if( Psyllid_ENABLE_EXECUTABLES )

    set( lib_dependencies 
        PsyllidUtility
        PsyllidData
        PsyllidNodes
    )
    
    set( programs
        single_value_daq
    )
    
    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
