cmake_minimum_required(VERSION 3.9)
project(kit VERSION 1.0.0 LANGUAGES C CXX)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug | Release
SET(CMAKE_BUILD_TYPE "Debug")

# SET(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions(-DDEBUG)

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")

MESSAGE("Identifying OS...")
if(WIN32)
  MESSAGE("This is Windows.")
elseif(APPLE)
  MESSAGE("This is MacOS.")
elseif(UNIX)
  MESSAGE("This is Linux.")
endif()

include(CheckIncludeFile)
check_include_file("unistd.h" HAVE_UNISTD_H)

include_directories(
  "${PROJECT_SOURCE_DIR}"
)

add_library(kit "")
target_sources(kit
  PRIVATE
    "${PROJECT_SOURCE_DIR}/errcode.h"
    "${PROJECT_SOURCE_DIR}/fsm/fsm_prototype.cpp"
    "${PROJECT_SOURCE_DIR}/fsm/fsm_prototype.h"
    "${PROJECT_SOURCE_DIR}/2pc/coordinator.cpp"
    "${PROJECT_SOURCE_DIR}/2pc/coordinator.h"
    "${PROJECT_SOURCE_DIR}/2pc/cohort.cpp"
    "${PROJECT_SOURCE_DIR}/2pc/cohort.h"
)

add_executable(kitexample
  "${PROJECT_SOURCE_DIR}/example/use_fsm.cpp"
)
target_link_libraries(kitexample kit)

# install (TARGETS kit DESTINATION bin)
# install (FILES ${PROJECT_SOURCE_DIR}/fsm/fsm_prototype.h errcode.h DESTINATION include)
