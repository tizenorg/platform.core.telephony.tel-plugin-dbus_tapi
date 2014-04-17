CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(dbus-tapi-plugin C)

### Global setting ###
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(PKGCONFIGDIR "${PREFIX}/lib/pkgconfig" CACHE PATH PKGCONFIGDIR)
SET(CMAKE_INSTALL_PREFIX "${PREFIX}")

# Set required packages
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED glib-2.0 gobject-2.0 gio-2.0 gio-unix-2.0 tcore aul appsvc security-server libtzplatform-config)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/include/
	${CMAKE_SOURCE_DIR}/sat-manager/include/
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wdeclaration-after-statement -Wmissing-declarations -Wcast-align")

ADD_DEFINITIONS("-DFEATURE_TLOG_DEBUG")
ADD_DEFINITIONS("-DTCORE_LOG_TAG=\"DBUS_TAPI\"")
ADD_DEFINITIONS("-DPLUGIN_VERSION=${VERSION}")
ADD_DEFINITIONS("-DEXPORT_API=__attribute__((visibility(\"default\")))")

MESSAGE(${CMAKE_C_FLAGS})
MESSAGE(${CMAKE_EXE_LINKER_FLAGS})

SET(SRCS
	src/desc.c
	src/dtapi_util.c
	src/dtapi_main.c
	src/dtapi_call.c
	src/dtapi_gps.c
	src/dtapi_manager.c
	src/dtapi_modem.c
	src/dtapi_network.c
	src/dtapi_phonebook.c
	src/dtapi_sap.c
	src/dtapi_sat.c
	src/dtapi_sim.c
	src/dtapi_sms.c
	src/dtapi_ss.c
	sat-manager/src/sat_manager.c
	sat-manager/src/sat_ui_support.c
#	sat-manager/src/sat_util.c
)

SET(INTROSPECTION_PATH ${CMAKE_SOURCE_DIR}/introspection)

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/generated-code.c
	COMMAND gdbus-codegen
	--interface-prefix org.tizen.telephony.
	--generate-c-code generated-code
	--c-namespace Telephony
	--c-generate-object-manager
	${INTROSPECTION_PATH}/manager.xml
	${INTROSPECTION_PATH}/network.xml
	${INTROSPECTION_PATH}/sim.xml
	${INTROSPECTION_PATH}/phonebook.xml
	${INTROSPECTION_PATH}/sap.xml
	${INTROSPECTION_PATH}/sat.xml
	${INTROSPECTION_PATH}/sms.xml
	${INTROSPECTION_PATH}/call.xml
	${INTROSPECTION_PATH}/ss.xml
	${INTROSPECTION_PATH}/modem.xml
	${INTROSPECTION_PATH}/gps.xml
	COMMENT "Generating GDBus .c/.h")

# library build
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS} ${CMAKE_BINARY_DIR}/generated-code.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "" OUTPUT_NAME ${PROJECT_NAME})

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/res/tapi.conf.in ${CMAKE_CURRENT_SOURCE_DIR}/res/tapi.conf)

# install
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/tapi.conf DESTINATION ${PREFIX}/etc/dbus-1/system.d)
INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/telephony/plugins)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION /usr/share/license RENAME tel-plugin-dbus_tapi)
