#pragma once

#include "kernel/kernel.hpp"
#include "kernel/process.hpp"

#include <mutex>
#include <sol/sol.hpp>

namespace strawbie {
  inline void mksystem(sol::state& lua, strawbie::Kernel* kernel, strawbie::Process* proc) {
    sol::table system = lua.create_named_table("system");

    system["send"] = [kernel, proc](unsigned int pid, std::string name, sol::object data) {
      kernel->send(pid, name, data, proc->pid);
    };

    system["poll"] = [&lua, kernel, proc]() {
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
  }
}
