cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)

project(StrawbieOS LANGUAGES C CXX)

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

# Set up Lua (pain, no CMake build)
FetchContent_Declare(
  lua_source
  URL https://www.lua.org/ftp/lua-5.4.6.tar.gz
)

FetchContent_MakeAvailable(lua_source)

file(GLOB LUA_SOURCES "${lua_source_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM LUA_SOURCES
    "${lua_source_SOURCE_DIR}/src/lua.c"
    "${lua_source_SOURCE_DIR}/src/luac.c"
)

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC "${lua_source_SOURCE_DIR}/src")

if(UNIX)
    target_link_libraries(lua PUBLIC dl m)
    target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
endif()

# Sol2
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG develop
)
FetchContent_MakeAvailable(sol2)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/kernel/kernel.hpp
  src/kernel/kernel.cpp
  src/kernel/modules/vfs.hpp
  src/kernel/modules/module.hpp
  src/kernel/modules/vfs.cpp
  src/kernel/process.hpp
  src/exceptions.hpp
  src/api/env.hpp
  src/api/graphics.hpp
  src/api/mksystem.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  raylib
  lua
  sol2::sol2
)

add_custom_target(copy_resources ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/bootstrap/rootfs"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/rootfs"
  COMMENT "Copying assets and rootfs to output directory..."
)

add_dependencies(copy_resources ${PROJECT_NAME})
