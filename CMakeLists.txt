find_package(Qt5 COMPONENTS Core Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_library(QtZeroConf STATIC
    qzeroconfservice.h
    qzeroconfglobal.h
    qzeroconfservice.cpp
    qzeroconf.h
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_sources(QtZeroConf PRIVATE
        avahi-qt/qt-watch.h
        avahi-qt/qt-watch_p.h
        avahi-qt/qt-watch.cpp
        avahiclient.cpp
    )
    target_link_libraries(QtZeroConf PRIVATE libavahi)
endif()

if(APPLE)
    target_sources(QtZeroConf PRIVATE
        bonjour_p.h
        bonjour.cpp
    )
    find_library(CoreServices CoreServices)
    target_link_libraries(QtZeroConf PUBLIC ${CoreServices})
endif()

target_include_directories(QtZeroConf PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(QtZeroConf PUBLIC Qt5::Core Qt5::Network)

if(WIN32)
    target_sources(QtZeroConf PRIVATE
        qzeroconf.h
        bonjour_p.h
        bonjour.cpp
        bonjour-sdk/dnssd_clientlib.c
        bonjour-sdk/dnssd_clientstub.c
        bonjour-sdk/dnssd_ipc.c
        )
    find_library(legacy_stdio_definitions legacy_stdio_definitions)
    target_link_libraries(QtZeroConf PUBLIC ws2_32 ${legacy_stdio_definitions})
    target_compile_definitions(QtZeroConf PRIVATE
        -DQZEROCONF_STATIC
        -DWIN32
        -D_USRDLL
        -DMDNS_DEBUGMSGS=0
        -DWIN32_LEAN_AND_MEAN
        -DUSE_TCP_LOOPBACK
        -D_NO_CRT_STDIO_INLINE
        -D_CRT_SECURE_NO_DEPRECATE
        -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
        -DNOT_HAVE_SA_LEN)
    target_compile_definitions(QtZeroConf PUBLIC -DQZEROCONF_STATIC)
    target_include_directories(QtZeroConf PRIVATE "${CMAKE_CURRENT_LIST_DIR}/bonjour-sdk")
endif()


if(ANDROID)
     set(ACM "${CMAKE_CURRENT_LIST_DIR}/avahi-common")
     set(ACR "${CMAKE_CURRENT_LIST_DIR}/avahi-core")
     target_sources(QtZeroConf PRIVATE
        qzeroconf.h
        avahi-qt/qt-watch.h
        avahi-qt/qt-watch_p.h
        avahicore.cpp
        avahi-qt/qt-watch.cpp
        ${ACM}/address.c
        ${ACM}/alternative.c
        ${ACM}/domain.c
        ${ACM}/error.c
        ${ACM}/i18n.c
        ${ACM}/malloc.c
        ${ACM}/rlist.c
        ${ACM}/simple-watch.c
        ${ACM}/strlst.c
        ${ACM}/thread-watch.c
        ${ACM}/timeval.c
        ${ACM}/utf8.c
        ${ACR}/addr-util.c
        ${ACR}/announce.c
        ${ACR}/browse.c
        ${ACR}/browse-dns-server.c
        ${ACR}/browse-domain.c
        ${ACR}/browse-service.c
        ${ACR}/browse-service-type.c
        ${ACR}/cache.c
        ${ACR}/dns.c
        ${ACR}/domain-util.c
        ${ACR}/entry.c
        ${ACR}/fdutil.c
        ${ACR}/hashmap.c
        ${ACR}/iface.c
        ${ACR}/iface-linux.c
        ${ACR}/log.c
        ${ACR}/multicast-lookup.c
        ${ACR}/netlink.c
        ${ACR}/prioq.c
        ${ACR}/probe-sched.c
        ${ACR}/querier.c
        ${ACR}/query-sched.c
        ${ACR}/resolve-address.c
        ${ACR}/resolve-host-name.c
        ${ACR}/resolve-service.c
        ${ACR}/response-sched.c
        ${ACR}/rr.c
        ${ACR}/rrlist.c
        ${ACR}/server.c
        ${ACR}/socket.c
        ${ACR}/timeeventq.c
        ${ACR}/util.c
        ${ACR}/wide-area.c
)
    target_compile_definitions(QtZeroConf PRIVATE HAVE_STRLCPY GETTEXT_PACKAGE HAVE_NETLINK)
endif()
