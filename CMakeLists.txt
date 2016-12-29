# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required (VERSION 3.1)

# Define the project
cmake_policy( SET CMP0048 NEW ) # version in project()
project( Psyllid VERSION 1.0.2)

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

# add an option to perform iterator time profiling
option( Psyllid_ENABLE_ITERATOR_TIMING "Flag to enable iterator time profiling" FALSE )
if( Psyllid_ENABLE_ITERATOR_TIMING )
    add_definitions( -DENABLE_ITERATOR_TIMING )
else( Psyllid_ENABLE_ITERATOR_TIMING )
    remove_definitions( -DENABLE_ITERATOR_TIMING )
endif( Psyllid_ENABLE_ITERATOR_TIMING )


################
# dependencies #
################

# Boost
# Boost (1.48 required for container; scarab minimum is 1.46)
find_package( Boost 1.48.0 REQUIRED )

# Midge
pbuilder_add_submodule( Midge midge )
midge_process_options()

# Dripline
add_definitions( -DDRIPLINE_AUTH_FILE=.project8_authentications.json )
pbuilder_add_submodule( Dripline dripline-cpp )

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

