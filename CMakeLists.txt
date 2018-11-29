# Minimum cmake verison 3.1 required for the variable CMAKE_CXX_STANDARD
cmake_minimum_required (VERSION 3.1)

# Define the project
cmake_policy( SET CMP0048 NEW ) # version in project()
project( Psyllid VERSION 1.9.1 )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/midge/scarab/cmake )
include( PackageBuilder )

list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/midge/cmake )
include( MidgeUtility )
list( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
include( Psyllid )

pbuilder_prepare_project()


#########
# flags #
#########

set_option( USE_CPP11 TRUE )

# add an option to perform iterator time profiling
option( Psyllid_ENABLE_ITERATOR_TIMING "Flag to enable iterator time profiling" FALSE )
if( Psyllid_ENABLE_ITERATOR_TIMING )
    add_definitions( -DENABLE_ITERATOR_TIMING )
else( Psyllid_ENABLE_ITERATOR_TIMING )
    remove_definitions( -DENABLE_ITERATOR_TIMING )
endif( Psyllid_ENABLE_ITERATOR_TIMING )

set( default_flag FALSE )
if( UNIX AND NOT APPLE )
    set( default_flag TRUE )
endif( UNIX AND NOT APPLE )
option( Psyllid_ENABLE_FPA "Flag to enable the fast-packet-acquisition interface (requires root)" ${default_flag} )
if( Psyllid_ENABLE_FPA AND UNIX AND NOT APPLE )
    set( Psyllid_BUILD_FPA TRUE )
    add_definitions( -DBUILD_FPA )
else( Psyllid_ENABLE_FPA AND UNIX AND NOT APPLE )
    set( Psyllid_BUILD_FPA FALSE )
    remove_definitions( -DBUILD_FPA )
endif( Psyllid_ENABLE_FPA AND UNIX AND NOT APPLE )

################
# dependencies #
################

# FFTW
option( Psyllid_ENABLE_FFTW "Flag to enable FFTW features" TRUE )
if (Psyllid_ENABLE_FFTW)
    find_package(FFTW REQUIRED)
else (Psyllid_ENABLE_FFTW)
    set (FFTW_FOUND FALSE)
endif (Psyllid_ENABLE_FFTW)
if (FFTW_FOUND)
    add_definitions(-DFFTW_FOUND)
    pbuilder_add_ext_libraries (${FFTW_LIBRARIES})
    if (FFTW_THREADS_FOUND)
        set (FFTW_NTHREADS 1 CACHE STRING "Number of threads to use for FFTW processes")
        add_definitions (-DFFTW_NTHREADS=${FFTW_NTHREADS})
        message (STATUS "FFTW configured to use up to ${FFTW_NTHREADS} threads.")
    else (FFTW_THREADS_FOUND)
        remove_definitions (-DFFTW_NTHREADS=${FFTW_NTHREADS})
    endif (FFTW_THREADS_FOUND)
else (FFTW_FOUND)
    message(STATUS "Building without FFTW")
    remove_definitions(-DFFTW_FOUND)
    remove_definitions (-DFFTW_NTHREADS=${FFTW_NTHREADS})
endif (FFTW_FOUND)
include_directories (${FFTW_INCLUDE_DIR})

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
# we need the definitions for HAS_ATTR_IFC and HAS_GRP_IFC from Monarch to use the proper class interfaces in M3Header and M3Stream
set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} APPEND PROPERTY COMPILE_DEFINITIONS ${Monarch_COMPILE_DEFINITIONS} )

# tk_spline
include_directories( BEFORE ${PROJECT_SOURCE_DIR}/external/tk_spline )


###########
# psyllid #
###########

# add include directories
include_directories( BEFORE
    source/control
    source/data
    source/utility
)


# build this project
add_subdirectory( source/utility )
add_subdirectory( source/data )
add_subdirectory( source/control )

add_subdirectory( source/applications )
add_subdirectory( source/test )

pbuilder_variables_for_parent()
