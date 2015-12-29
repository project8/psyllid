########
# test #
########

set( midge_test_dependencies
    _psyllid_utility
    _psyllid_data
    _psyllid_nodes
)

set( midge_test_programs
    test_event_builder
    test_server
    test_udp_receiver
)

midge_executables( test )


