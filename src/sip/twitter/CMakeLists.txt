project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

set( twitterSources
    twitter.cpp
    tomahawkoauthtwitter.cpp
)

set( twitterHeaders
    twitter.h
	tomahawkoauthtwitter.h
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} .. 
   ${QT_INCLUDE_DIR}
   ${CMAKE_SOURCE_DIR}/thirdparty/qtweetlib/src
)

qt4_wrap_cpp( twitterMoc ${twitterHeaders} )
add_library( sip_twitter SHARED ${twitterSources} ${twitterMoc} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
	${OS_SPECIFIC_LINK_LIBRARIES}
	"winmm.dll"
        "iphlpapi.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/src/libtomahawk/libtomahawklib.dll"
)
ENDIF( WIN32 )

target_link_libraries( sip_twitter
	${QT_LIBRARIES}
	${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS sip_twitter DESTINATION lib )