#include "kernel/kernel.h"
#include "kernel/modules/module.h"
#include "kernel/modules/vfs.h"
#include "kernel/process.h"
#include "lauxlib.h"
#include "lua.h"
#include "raylib.h"

#include <memory>
#include <sol/sol.hpp>
#include <thread>

namespace {
  const int CPU_INSTRUCTION_TIME = 10'000;
  const int CPU_INSTRUCTION_LIMIT = 5'000'000;

  void watchHook(lua_State* L, lua_Debug* dbg) {
    lua_getfield(L, LUA_REGISTRYINDEX, "__strawbie_proc");
    strawbie::Process* proc = static_cast<strawbie::Process*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!proc) return;

    proc->instructions += CPU_INSTRUCTION_TIME;
    if (proc->instructions >= CPU_INSTRUCTION_LIMIT) {
      proc->alive = false;
    }

    if (!proc->alive) {
      lua_sethook(L, nullptr, 0, 0);
      luaL_error(L, "Process terminated by kernel (SIGKILL)");
    }
  }
}

void strawbie::Kernel::boot() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->init();
  }
}

void strawbie::Kernel::tick() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->tick();
  }

  float dt = GetFrameTime();
  for (auto it = this->mProcesses.begin(); it != this->mProcesses.end(); ) {
    Process& proc = *it->second;
    if (!proc.alive) {
      it = this->mProcesses.erase(it);
    } else {
      proc.delta = dt;
      proc.frame.release();
      ++it;
    }
  }

  BeginDrawing();
    ClearBackground(WHITE);
    // TODO: stuff
  EndDrawing();
}

void strawbie::Kernel::shutdown() {
  for (auto& [pid, proc] : this->mProcesses) {
    proc->alive = false;
    proc->frame.release();
  }

  this->mProcesses.clear();
}

void strawbie::Kernel::kworker(strawbie::Process* process, const std::string content) {
  sol::state lua;

  // TODO: Custom ENV, better handling, other BS
  lua.open_libraries();

  lua_pushlightuserdata(lua.lua_state(), process);
  lua_setfield(lua.lua_state(), LUA_REGISTRYINDEX, "__strawbie_proc");

  lua_sethook(lua.lua_state(), watchHook, LUA_MASKCOUNT, CPU_INSTRUCTION_TIME);

  std::string name = "@" + process->name;
  sol::protected_function_result result = lua.safe_script(content, sol::script_pass_on_error, name);
  if (!result.valid()) {
    sol::error err = result;

    process->alive = false;
    std::cout << err.what() << std::endl;
    return;
  }

  sol::protected_function init   = (lua)["init"];
  sol::protected_function update = (lua)["update"];
  sol::protected_function draw   = (lua)["draw"];

  process->instructions = 0;

  if (init.valid()) {
    sol::protected_function_result res = init();
    if (!res.valid()) {
      sol::error err = res;

      process->alive = false;
      std::cout << err.what() << std::endl;

      return;
    }
  }

  // Die if its an immediate app
  if (!update.valid() && !draw.valid()) {
    process->alive = false;
    return;
  }

  while (process->alive) {
    process->frame.acquire();
    if (!process->alive) return;

    process->instructions = 0;

    if (update.valid()) {
      sol::protected_function_result res = update(process->delta);
      if (!res.valid()) {
        sol::error err = res;

        process->alive = false;
        std::cout << err.what() << std::endl;

        break;
      }
    }

    if (draw.valid()) {
      sol::protected_function_result res = draw(process->delta);
      if (!res.valid()) {
        sol::error err = res;

        process->alive = false;
        std::cout << err.what() << std::endl;

        break;
      }
    }
  }
}

unsigned int strawbie::Kernel::process(std::string_view path) {
  strawbie::VirtualFS* vfs = this->get<strawbie::VirtualFS>();

  std::string clean = vfs->normalise(path);
  std::string content = vfs->read(clean);

  unsigned int pid = this->mPid++;

  std::unique_ptr<Process> proc = std::make_unique<Process>();

  proc->pid = pid;
  proc->name = clean;

  Process* procPtr = proc.get();
  this->mProcesses.emplace(pid, std::move(proc));

  procPtr->thread = std::jthread(&strawbie::Kernel::kworker, this, procPtr, std::move(content));

  return pid;
}
