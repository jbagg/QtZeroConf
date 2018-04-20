find_package(Qt5 COMPONENTS Core Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_library(QtZeroConf
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

if(WIN32)
    target_sources(QtZeroConf PRIVATE
        qzeroconf.h
        bonjour_p.h
        bonjour.cpp
        bonjour-sdk/dnssd_clientlib.c
        bonjour-sdk/dnssd_clientstub.c
        bonjour-sdk/dnssd_ipc.c
        )
    target_link_libraries(QtZeroConf PUBLIC -lws2_32 -lwsock32)
    target_compile_definitions(QtZeroConf PRIVATE
        -DQT_BUILD_ZEROCONF_LIB
        -DWIN32
        -D_USRDLL
        -DMDNS_DEBUGMSGS=0
        -DWIN32_LEAN_AND_MEAN
        -DUSE_TCP_LOOPBACK
        -D_CRT_SECURE_NO_DEPRECATE
        -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
        -DNOT_HAVE_SA_LEN)
    target_include_directories(QtZeroConf PRIVATE "${CMAKE_CURRENT_LIST_DIR}/bonjour-sdk")
endif()

target_include_directories(QtZeroConf PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(QtZeroConf PUBLIC Qt5::Core Qt5::Network)
