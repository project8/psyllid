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

# add an option to build docs
option( Psyllid_BUILD_DOCS "Enable make targets for both doxygen and sphinx (requires Doxygen, Sphinx, and Breathe)" FALSE )


################
# dependencies #
################

# Midge
pbuilder_add_submodule( Midge midge )
midge_process_options()

# Dripline
pbuilder_add_submodule( Dripline dripline/cpp )

# Monarch
pbuilder_add_submodule( Monarch monarch )


###########
# psyllid #
###########

# add include directories
include_directories( BEFORE
    source/control
    source/data
    source/daq
    source/utility
)


# build this project
add_subdirectory( source/utility )
add_subdirectory( source/data )
add_subdirectory( source/daq )
add_subdirectory( source/control )

add_subdirectory( source/applications )
add_subdirectory( source/test )

if( Psyllid_BUILD_DOCS )
    add_subdirectory( documentation )
endif( Psyllid_BUILD_DOCS )

