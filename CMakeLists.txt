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
pkg_check_modules(pkgs REQUIRED glib-2.0 dbus-1 dbus-glib-1 gthread-2.0 tcore dlog tapi aul security-server)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wdeclaration-after-statement -Wmissing-declarations -Wredundant-decls -Wcast-align")

ADD_DEFINITIONS("-DFEATURE_DLOG_DEBUG")
ADD_DEFINITIONS("-DTCORE_LOG_TAG=\"DBUSTAPI\"")

MESSAGE(${CMAKE_C_FLAGS})
MESSAGE(${CMAKE_EXE_LINKER_FLAGS})

SET(SRCS
		src/desc-dbus.c
		src/ts_noti.c
		src/ts_svr_req.c
		src/call.c
		src/cfg.c
		src/cdmadata.c
		src/gprs.c
		src/gps.c
		src/misc.c
		src/network.c
		src/omadm.c
		src/power.c
		src/productivity.c
		src/sat.c
		src/sat_mgr.c
		src/sim.c
		src/sms.c
		src/sound.c
		src/ss.c
		src/util.c
)


ADD_CUSTOM_TARGET(tapi-lib-stub.h
		COMMAND dbus-binding-tool --prefix= --mode=glib-server --output=${CMAKE_CURRENT_SOURCE_DIR}/include/tapi-lib-stub.h ${CMAKE_CURRENT_SOURCE_DIR}/src/tapi-lib.xml)


# library build
ADD_LIBRARY(dbus-tapi-plugin SHARED ${SRCS})
TARGET_LINK_LIBRARIES(dbus-tapi-plugin ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(dbus-tapi-plugin PROPERTIES PREFIX "" OUTPUT_NAME dbus-tapi-plugin)
ADD_DEPENDENCIES(dbus-tapi-plugin tapi-lib-stub.h)



# install
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/tapi.conf DESTINATION ${PREFIX}/etc/dbus-1/system.d)
INSTALL(TARGETS dbus-tapi-plugin
		LIBRARY DESTINATION lib/telephony/plugins)
