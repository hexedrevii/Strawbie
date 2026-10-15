#pragma once

#include "api/mksystem.hpp"
#include "kernel/kernel.hpp"
#include "kernel/process.hpp"
#include "api/graphics.hpp"

#include <sol/sol.hpp>
#include <sol/types.hpp>

namespace strawbie {
  inline void mkenv(sol::state& lua, strawbie::Kernel* kernel, strawbie::Process* proc) {
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);

    lua["dofile"] = sol::lua_nil;
    lua["loadfile"] = sol::lua_nil;
    lua["loadstring"] = sol::lua_nil;
    lua["collectgarbage"] = sol::lua_nil;

    sol::table package = lua.create_named_table("package");
    package["loaded"] = lua.create_table();
    package["path"] = "./?.lua;/lib/?.lua;/lib/?/init.lua;/sys/lib/?.lua;/sys/lib/?/init.lua";

    lua["require"] = [](std::string module, sol::this_state s) -> sol::object {
      sol::state_view lua(s);

      sol::table package = lua["package"];
      sol::table loaded = package["loaded"];

      sol::object existing = loaded[module];
      if (existing != sol::lua_nil) {
        return existing;
      }

      std::string search = module;
      std::replace(search.begin(), search.end(), '.', '/');

      std::string paths = package["path"];
      std::stringstream ss(paths);

      std::string templatePath;

      std::string content = "";
      std::string final = "";

      // I... do not have a CWD yet.

      return sol::lua_nil;
    };

    mkgraphics(lua, kernel, proc);
    mksystem(lua, kernel, proc);
  }
}
