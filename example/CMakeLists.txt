project(QtZeroConfExample)
cmake_minimum_required(VERSION 2.8.11)

find_package(Qt5 COMPONENTS Gui Widgets REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find QtZeroConf library if we're building standalone
if(NOT TARGET QtZeroConf)
    find_package(QtZeroConf CONFIG REQUIRED)
endif()

add_executable(QtZeroConfExample
    window.h
    window.cpp
    main.cpp
)

target_link_libraries(QtZeroConfExample
    QtZeroConf
    Qt5::Gui Qt5::Widgets
)
