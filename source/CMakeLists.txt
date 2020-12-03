# CMakeLists.txt for psyllid/library

if( TARGET TKSpline )
    message( STATUS "######## HAVE tkspline" )
else()
    message( STATUS "######## DO NOT HAVE tkspline" )
endif()

pbuilder_use_sm_library( SandflyUtility Sandfly )
pbuilder_use_sm_library( SandflyControl Sandfly )
pbuilder_use_sm_library( Monarch3 Monarch )

add_subdirectory( utility )
add_subdirectory( control )
add_subdirectory( data )
add_subdirectory( daq )

pbuilder_component_install_and_export(
    COMPONENT Library
    LIBTARGETS PsyllidUtility PsyllidControl PsyllidData PsyllidDAQ
)
