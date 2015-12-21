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


# add include directories

# build core library
midge_build_library()

# build this project

# build core executable
midge_build_executables()
