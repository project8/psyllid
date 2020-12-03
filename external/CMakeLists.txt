# CMakeLists.txt for psyllid/external

###########
# TK Spline
###########

add_library( TKSpline INTERFACE )

set_target_properties( TKSpline PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/tk_spline"
)
