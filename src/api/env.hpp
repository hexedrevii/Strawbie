#pragma once

#include "kernel/kernel.hpp"
#include "kernel/process.hpp"
#include "api/graphics.hpp"

#include <sol/sol.hpp>

namespace strawbie {
  inline void mkenv(sol::state& lua, strawbie::Kernel* kernel, strawbie::Process* proc) {
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);

    lua["dofile"] = sol::lua_nil;
    lua["loadfile"] = sol::lua_nil;
    lua["loadstring"] = sol::lua_nil;
    lua["collectgarbage"] = sol::lua_nil;

    lua["require"] = sol::lua_nil;

    mkgraphics(lua, kernel, proc);
  }
}
