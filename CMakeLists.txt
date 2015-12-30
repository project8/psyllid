cmake_minimum_required (VERSION 3.0)

# Define the project
cmake_policy( SET CMP0048 NEW ) # version in project()
project (psyllid VERSION 0.1.0)

include( midge/cmake/MidgeUtility.cmake )
midge_prepare_project()

message( STATUS "midge project name: ${midge_project_name}" )

#########
# flags #
#########



################
# dependencies #
################

# Midge
midge_build_library()


# Monarch
add_subdirectory( monarch )
pbuilder_add_submodule_libraries( ${Monarch_LIBRARIES} )
include_directories( ${Monarch_DEP_INCLUDE_DIRS} )


###########
# psyllid #
###########

# add include directories
include_directories( BEFORE
    data
    nodes
    utility
)


# build this project
add_subdirectory( utility )
add_subdirectory( data )
add_subdirectory( nodes )

add_subdirectory( applications )
add_subdirectory( test )

