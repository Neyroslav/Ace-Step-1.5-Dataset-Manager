cmake_minimum_required(VERSION 3.16)
project(MusicDatasetManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

add_executable(MusicDatasetManager
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/audioitemwidget.h
    src/audioitemwidget.cpp
    src/plaintextedit.h
    src/plaintextedit.cpp
    src/resizabletextedit.h
    src/resizabletextedit.cpp
)

target_link_libraries(MusicDatasetManager
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Multimedia
)
