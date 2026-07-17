cmake_minimum_required(VERSION 3.24)

project(aegis VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Concurrent Network)
find_package(Qt6Keychain QUIET)
find_package(unofficial-libgit2 CONFIG QUIET)
if(TARGET unofficial::libgit2::libgit2 AND NOT TARGET Libgit2::Libgit2)
  add_library(Libgit2::Libgit2 INTERFACE IMPORTED)
  target_link_libraries(Libgit2::Libgit2 INTERFACE
    unofficial::libgit2::libgit2)
else()
  find_package(Libgit2 1.7 QUIET)
endif()

qt_standard_project_setup(REQUIRES 6.6)

option(AEGIS_BUILD_GUI "Build the Qt Quick application when development modules exist" ON)

set(AEGIS_GUI_AVAILABLE OFF)
if(AEGIS_BUILD_GUI)
  find_package(Qt6 6.6 QUIET COMPONENTS Gui Qml Quick QuickControls2 Svg)
  if(TARGET Qt6::Gui AND TARGET Qt6::Qml AND TARGET Qt6::Quick
     AND TARGET Qt6::QuickControls2 AND TARGET Qt6::Svg)
    set(AEGIS_GUI_AVAILABLE ON)
  else()
    message(WARNING
      "Qt Quick development modules are unavailable; building aegis_core and tests only")
  endif()
endif()

include(cmake/AegisWarnings.cmake)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  find_package(Qt6 6.6 REQUIRED COMPONENTS Test)
  add_subdirectory(tests)
endif()
