cmake_minimum_required(VERSION 3.14)
project(QtZeroConfExample)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Gui Widgets REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find QtZeroConf library if we're building standalone
if(NOT TARGET QtZeroConf)
    find_package(QtZeroConf CONFIG REQUIRED)
endif()

# Check if it's Android and if Qt is >= 6.6.0
if(ANDROID AND QT_VERSION_MAJOR GREATER_EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        window.h
        window.cpp
        main.cpp
    )

    if(NOT EXISTS ${ANDROID_OPENSSL_PATH})
        message(WARNING "Cannot find OpenSSL for Android. Path: ${ANDROID_OPENSSL_PATH}")
        message(FATAL_ERROR "Please set ANDROID_OPENSSL_PATH to the path of OpenSSL for Android.")
    endif()

    message(STATUS "Found OpenSSL for Android. Path: ${ANDROID_OPENSSL_PATH}")
    set_property(TARGET ${PROJECT_NAME}
        PROPERTY QT_ANDROID_TARGET_SDK_VERSION
        34
    )

    set_property(TARGET ${PROJECT_NAME}
        PROPERTY QT_ANDROID_EXTRA_LIBS
        ${ANDROID_OPENSSL_PATH}/libcrypto_3.so
        ${ANDROID_OPENSSL_PATH}/libssl_3.so
    )

    set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    )

    set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/src/android-build)
else ()
    add_executable(${PROJECT_NAME}
        window.h
        window.cpp
        main.cpp
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    QtZeroConf
    Qt${QT_VERISON_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets
)
