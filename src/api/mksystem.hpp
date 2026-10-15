#pragma once

#include "kernel/kernel.hpp"
#include "kernel/modules/vfs.hpp"
#include "kernel/process.hpp"

#include <mutex>
#include <optional>
#include <sol/sol.hpp>
#include <sol/types.hpp>

namespace strawbie {
  inline void mksystem(sol::state& lua, strawbie::Kernel* kernel, strawbie::Process* proc) {
    sol::table system = lua.create_named_table("system");

    system["send"] = [kernel, proc](unsigned int pid, std::string name, sol::object data) {
      kernel->send(pid, name, data, proc->pid);
    };

    system["poll"] = [kernel, proc](sol::this_state s) {
      sol::state_view lua(s);

      sol::table result = lua.create_table();

      std::lock_guard<std::mutex> lock(proc->ipcLock);

      int index = 1;
      for (strawbie::Message& msg : proc->ipc) {
        sol::table msgt = lua.create_table();

        msgt["sender"] = msg.sender;
        msgt["name"] = msg.name;
        msgt["payload"] = kernel->deserialise(lua, msg.payload);

        result[index++] = msgt;
      }

      proc->ipc.clear();
      return result;
    };

    system["getcwd"] = [proc]() -> std::string {
      return proc->cwd;
    };

    system["setcwd"] = [kernel, proc](std::string path) {
      kernel->setCWD(proc->pid, path);
    };

    system["spawn"] = [kernel, proc](std::string path, std::optional<std::string> cwd) -> unsigned int {
      strawbie::VirtualFS *vfs = kernel->get<strawbie::VirtualFS>();
      if (vfs->normalise(path).starts_with("/sys/") && !(proc->priviledges & Priviledges::MASTER)) {
        throw sol::error("Cannot launch process: Permission Denied.");
      }

      return kernel->process(path, cwd);
    };

    if (proc->priviledges & Priviledges::MASTER) {
      system["register"] = [kernel, proc](std::string name) {
        kernel->addRegistry(name, proc->pid);
      };
    }
  }
}
