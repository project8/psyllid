# Minimum cmake verison 3.9 required for Monarch
cmake_minimum_required (VERSION 3.12)

# Define the project
cmake_policy( SET CMP0048 NEW ) # version in project()
project( Psyllid VERSION 2.1.1 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/sandfly/cmake )
include( sandfly )


#########
# flags #
#########

set( default__fpa_flag FALSE )
if( UNIX AND NOT APPLE )
    set( default__fpa_flag TRUE )
endif( UNIX AND NOT APPLE )

set_option( USE_CPP11 TRUE )

option( Psyllid_ENABLE_ITERATOR_TIMING "Flag to enable iterator time profiling" FALSE )
option( Psyllid_ENABLE_FPA "Flag to enable the fast-packet-acquisition interface (requires root)" ${default__fpa_flag} )
option( Psyllid_ENABLE_STREAMED_FREQUENCY_OUTPUT "Flag to enable building node for streaming frequency data (inproper monarch usage)" FALSE )
option( Psyllid_ENABLE_FFTW "Flag to enable FFTW features" TRUE )
option( Psyllid_ENABLE_EXAMPLES "Flag to enable building of examples" FALSE )

# add an option to perform iterator time profiling
if( Psyllid_ENABLE_ITERATOR_TIMING )
    add_definitions( -DENABLE_ITERATOR_TIMING )
else( Psyllid_ENABLE_ITERATOR_TIMING )
    remove_definitions( -DENABLE_ITERATOR_TIMING )
endif( Psyllid_ENABLE_ITERATOR_TIMING )

# FPA should only be available for a linux machine, and not apple
if( Psyllid_ENABLE_FPA AND UNIX AND NOT APPLE )
    set( Psyllid_BUILD_FPA TRUE )
    add_definitions( -DBUILD_FPA )
else( Psyllid_ENABLE_FPA AND UNIX AND NOT APPLE )
    set( Psyllid_BUILD_FPA FALSE )
    remove_definitions( -DBUILD_FPA )
endif( Psyllid_ENABLE_FPA AND UNIX AND NOT APPLE )

# Control executable build
set_option( Midge_ENABLE_EXECUTABLES FALSE )
set_option( Sandfly_ENABLE_EXECUTABLES FALSE )


################
# dependencies #
################

set( PUBLIC_EXT_LIBS )
set( PRIVATE_EXT_LIBS )

# FFTW
if( Psyllid_ENABLE_FFTW)
    find_package( FFTW REQUIRED MODULE )
else ( Psyllid_ENABLE_FFTW )
    set( FFTW_FOUND FALSE )
endif( Psyllid_ENABLE_FFTW )
if ( FFTW_FOUND )
    #include_directories( ${FFTW_INCLUDE_DIR} )
    add_definitions( -DFFTW_FOUND )
    list( APPEND PUBLIC_EXT_LIBS ${FFTW_LIBRARIES} )
    if( FFTW_THREADS_FOUND )
        set( FFTW_NTHREADS 1 CACHE STRING "Number of threads to use for FFTW processes" )
        add_definitions( -DFFTW_NTHREADS=${FFTW_NTHREADS} )
        list( APPEND PUBLIC_EXT_LIBS ${FFTW_THREADS_LIBRARIES} )
        message( STATUS "FFTW configured to use up to ${FFTW_NTHREADS} threads." )
    else( FFTW_THREADS_FOUND )
        remove_definitions( -DFFTW_NTHREADS=${FFTW_NTHREADS} )
    endif( FFTW_THREADS_FOUND )
else( FFTW_FOUND )
    message( STATUS "Building without FFTW" )
    set( Psyllid_ENABLE_FFTW FALSE )
    remove_definitions( -DFFTW_FOUND )
    remove_definitions( -DFFTW_NTHREADS=${FFTW_NTHREADS} )
endif( FFTW_FOUND )

# Boost
# Boost (1.48 required for container; scarab minimum is 1.46)
#find_package( Boost 1.48.0 REQUIRED )

# Sandfly
add_definitions( -DDRIPLINE_AUTH_FILE=~/.project8_authentications.json )

# Included external dependencies
# Including: tk_spline
add_subdirectory( external )
list( APPEND PRIVATE_EXT_LIBS TKSpline )


#####################
# prepare for build #
#####################

pbuilder_prepare_project()


##############
# submodules #
##############

# Sandfly
pbuilder_add_submodule( Sandfly sandfly )

# Monarch
pbuilder_add_submodule( Monarch monarch )


###########
# psyllid #
###########

# add include directories
include_directories( BEFORE
    source/control
    source/daq
    source/data
    source/utility
)

# build this project
add_subdirectory( source )

if( Psyllid_ENABLE_EXECUTABLES )
    add_subdirectory( source/applications )
endif()

if( Psyllid_ENABLE_TESTING )
    #add_subdirectory( source/test )
endif()

if( Psyllid_ENABLE_EXAMPLES )
    add_subdirectory( examples )
endif()


##################
# package config #
##################

configure_file( ${PROJECT_SOURCE_DIR}/PsyllidConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/PsyllidConfig.cmake @ONLY )

pbuilder_do_package_config()
