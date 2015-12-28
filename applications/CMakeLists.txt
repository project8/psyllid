################
# applications #
################

set( midge_apps_dependencies
    _psyllid_utility
    _psyllid_data
    _psyllid_nodes
)

set( midge_apps_programs
    single_value_daq
)

midge_executables( apps )


