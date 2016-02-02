################
# applications #
################

if( Psyllid_ENABLE_EXECUTABLES )

    set( lib_dependencies 
        PsyllidUtility
        PsyllidData
        PsyllidNodes
        PsyllidControl
    )
    
    set( programs
        #single_value_daq
        psyllid
    )
    
    pbuilder_executables( programs lib_dependencies )

endif( Psyllid_ENABLE_EXECUTABLES )
