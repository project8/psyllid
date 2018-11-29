# Psyllid.cmake
# Macros for building a project using Psyllid
# Author: B. LaRoque (based on Nymph.cmake by N. Oblath)

set (PSYLLID_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

macro (psyllid_build_executables)
    message( STATUS "%%%%%%% psyllid_dir is <${PSYLLID_DIR}>" )
    add_subdirectory (${PSYLLID_DIR}/source/applications)
endmacro ()
