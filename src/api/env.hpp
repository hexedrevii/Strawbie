#pragma once

#include "api/mksystem.hpp"
#include "kernel/kernel.hpp"
#include "kernel/modules/vfs.hpp"
#include "kernel/process.hpp"
#include "api/graphics.hpp"

#include <exception>
#include <sol/load_result.hpp>
#include <sol/protected_function_result.hpp>
#include <sol/sol.hpp>
#include <sol/types.hpp>
#include <string>

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

    lua["require"] = [proc, kernel](std::string module, sol::this_state s) -> sol::object {
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

      std::string errors = "Module " + module + " not found:";

      while (std::getline(ss, templatePath, ';')) {
        size_t pos = templatePath.find('?');

        if (pos != std::string::npos) {
          templatePath.replace(pos, 1, search);
        }

        if (templatePath.starts_with("./")) {
          std::string cwd = proc->cwd;
          if (!cwd.empty() && cwd.back() != '/') cwd += "/";

          templatePath = cwd + templatePath.substr(2);
        }

        bool found = false;
        std::string content;
        try {
          strawbie::VirtualFS *vfs = kernel->get<strawbie::VirtualFS>();
          content = vfs->read(templatePath);
          found = true;
        } catch (std::exception& e) {
          errors += "\n\tno file '" + templatePath + "'";
        }

        if (found) {
          sol::load_result chunk = lua.load(content, "@" + templatePath);
          if (!chunk.valid()) {
            sol::error err = chunk;
            throw sol::error("error loading module '" + module + "' from file '" + templatePath + "':\n\t" + err.what());
          }

          sol::protected_function_result result = chunk();
          if (!result.valid()) {
            sol::error err = result;
            throw sol::error(err.what());
          }

          sol::object ret = result.get<sol::object>();
          if (ret == sol::lua_nil) ret = sol::make_object(lua, true);

          loaded[module] = ret;
          return ret;
        }
      }

      throw sol::error(errors);
    };

    mkgraphics(lua, kernel, proc);
    mksystem(lua, kernel, proc);
  }
}
