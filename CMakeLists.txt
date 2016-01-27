cmake_minimum_required (VERSION 3.0)

# Define the project
cmake_policy( SET CMP0048 NEW ) # version in project()
project( Psyllid VERSION 0.1.0)

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/midge/scarab/cmake )
include( PackageBuilder )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/midge/cmake )
include( MidgeUtility )

pbuilder_prepare_project()


#########
# flags #
#########

set_option( USE_CPP11 TRUE )


################
# dependencies #
################

# Midge
pbuilder_add_submodule( Midge midge )

# Dripline
pbuilder_add_submodule( Dripline dripline/cpp )

# Monarch
pbuilder_add_submodule( Monarch monarch )


###########
# psyllid #
###########

# add include directories
include_directories( BEFORE
    control
    data
    nodes
    utility
)


# build this project
add_subdirectory( utility )
add_subdirectory( data )
add_subdirectory( nodes )
add_subdirectory( control )

add_subdirectory( applications )
add_subdirectory( test )
