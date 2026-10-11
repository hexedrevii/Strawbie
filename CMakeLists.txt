cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

project(StrawbieOS LANGUAGES CXX)

include(FetchContent)

find_package(raylib QUIET)
if (NOT raylib_FOUND)
  message(STATUS "Raylib not found! Building.")

  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib
    GIT_TAG master
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(raylib)
endif()

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/kernel/kernel.h
  src/kernel/kernel.cpp
  src/kernel/modules/vfs.h
  src/kernel/modules/module.h
  src/kernel/modules/vfs.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  raylib
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
  COMMENT "\nCopying assets to output directory..."
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/bootstrap/rootfs"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/rootfs"
  COMMENT "\nCopying root to output directory..."
)
