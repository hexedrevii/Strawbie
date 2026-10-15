#include "kernel/kernel.hpp"
#include "kernel/modules/module.hpp"
#include "kernel/modules/vfs.hpp"
#include "kernel/process.hpp"

#include "api/env.hpp"

#include "lauxlib.h"
#include "lua.h"
#include "raylib.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <sol/object.hpp>
#include <sol/sol.hpp>
#include <sol/types.hpp>
#include <thread>
#include <variant>
#include <vector>

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

  const int RESOLUTION_WIDTH = 420;
  const int RESOLUTION_HEIGHT = 270;

  const int MAX_PAYLOADS = 255;

  strawbie::IPCValue serialise(sol::object obj) {
    sol::type type = obj.get_type();

    if (type == sol::type::boolean) {
      return { obj.as<bool>() };
    } else if (type == sol::type::string) {
      return { obj.as<std::string>() };
    } else if (type == sol::type::number) {
      obj.push();
      bool isInteger = lua_isinteger(obj.lua_state(), -1);
      obj.pop();

      if (isInteger) {
        return { obj.as<int64_t>() };
      } else {
        return { obj.as<double>() };
      }
    } else if (type == sol::type::table) {
      std::map<strawbie::IPCKey, strawbie::IPCValue> map;
      sol::table table = obj.as<sol::table>();

      for (auto& [key, value] : table) {
        if (key.get_type() == sol::type::string) {
          map[key.as<std::string>()] = serialise(value);
        } else if (key.get_type() == sol::type::number) {
          key.push();
          bool isInteger = lua_isinteger(obj.lua_state(), -1);
          key.pop();

          if (isInteger) {
            map[key.as<int64_t>()] = serialise(value);
          } else {
            map[key.as<double>()] = serialise(value);
          }
        }
      }

      return { map };
    }

    return { std::monostate{} };
  }

  Texture2D resolution;
}

void strawbie::Kernel::registerProcess(std::string name, unsigned int pid) {
  this->mRegistry.emplace(name, pid);
}

unsigned int strawbie::Kernel::findProcess(std::string name) {
  if (!this->mRegistry.contains(name)) return 0;
  return this->mRegistry.at(name);
}

void strawbie::Kernel::boot() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->init();
  }

  Image black = GenImageColor(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, BLACK);

  resolution = LoadTextureFromImage(black);
  SetTextureFilter(resolution, TEXTURE_FILTER_POINT);

  UnloadImage(black);

  this->process("/sys/wm.lua");
  this->process("/dev/long.lua");
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
}

void strawbie::Kernel::draw() {
  float scale = std::min(
    GetScreenWidth() / RESOLUTION_WIDTH,
    GetScreenHeight() / RESOLUTION_HEIGHT
  );

  BeginDrawing();
    unsigned int wmpid = this->findProcess("display");
    if (wmpid != 0) {
      if (this->mProcesses.contains(wmpid)) {
        Process& wm = *this->mProcesses.at(wmpid);
        std::lock_guard<std::mutex> lock(wm.video.lock);
        if (wm.video.buffer.data != nullptr)
          UpdateTexture(resolution, wm.video.buffer.data);
      }
    }

    ClearBackground(BLACK);
    DrawTexturePro(
      resolution,
      (Rectangle) {
        0, 0,
        static_cast<float>(resolution.width),
        static_cast<float>(resolution.height)
      },
      (Rectangle) {
        (GetScreenWidth() - (RESOLUTION_WIDTH * scale)) * 0.5f,
        (GetScreenHeight() - (RESOLUTION_HEIGHT * scale)) * 0.5f,
        RESOLUTION_WIDTH * scale,
        RESOLUTION_HEIGHT * scale
      },
      (Vector2) {0,0},
      0, WHITE
    );
  EndDrawing();
}

void strawbie::Kernel::shutdown() {
  for (auto& [pid, proc] : this->mProcesses) {
    proc->alive = false;
    proc->frame.release();
  }

  this->mProcesses.clear();

  UnloadTexture(resolution);
}

void strawbie::Kernel::kworker(strawbie::Process* process, const std::string content) {
  sol::state lua;
  mkenv(lua, this, process);

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

    if (process->video.vram.data != nullptr) {
      std::lock_guard<std::mutex> lock(process->video.lock);
      int bytes = process->video.vram.width * process->video.vram.height * 4;
      memcpy(process->video.buffer.data, process->video.vram.data, bytes);
    }
  }
}

unsigned int strawbie::Kernel::process(std::string_view path, strawbie::Priviledges priviledges) {
  strawbie::VirtualFS* vfs = this->get<strawbie::VirtualFS>();

  std::string clean = vfs->normalise(path);
  std::string content = vfs->read(clean);

  // This MIIIGHT be a hack but... WHATEVER DUDE!
  if (clean.starts_with("/sys/")) {
    priviledges = priviledges | Priviledges::MASTER;
  }

  unsigned int pid = this->mPid++;

  std::unique_ptr<Process> proc = std::make_unique<Process>();

  proc->pid = pid;
  proc->name = clean;

  proc->priviledges = priviledges;

  Process* procPtr = proc.get();
  this->mProcesses.emplace(pid, std::move(proc));

  procPtr->thread = std::jthread(&strawbie::Kernel::kworker, this, procPtr, std::move(content));

  return pid;
}

strawbie::Process* strawbie::Kernel::getProcess(unsigned int pid) {
  if (!this->mProcesses.contains(pid)) {
    return nullptr;
  }

  return this->mProcesses.at(pid).get();
}

void strawbie::Kernel::send(unsigned int process, std::string name, sol::object payload, unsigned int sender) {
  if (!this->mProcesses.contains(process)) {
    // TODO: Maybe throw?
    return;
  }

  Process& proc = *this->mProcesses.at(process);

  std::lock_guard<std::mutex> lock(proc.ipcLock);

  if (proc.ipc.size() > MAX_PAYLOADS) {
    return;
  }

  IPCValue ipc = serialise(payload);
  Message msg = (Message){ .sender = sender, .name = name, .payload = ipc };

  proc.ipc.push_back(msg);
}

sol::object strawbie::Kernel::deserialise(sol::state_view lua, strawbie::IPCValue& val) {
  if (std::holds_alternative<bool>(val.data)) {
    return sol::make_object(lua, std::get<bool>(val.data));
  } else if (std::holds_alternative<std::string>(val.data)) {
    return sol::make_object(lua, std::get<std::string>(val.data));
  } else if (std::holds_alternative<double>(val.data)) {
    return sol::make_object(lua, std::get<double>(val.data));
  } else if (std::holds_alternative<int64_t>(val.data)) {
    return sol::make_object(lua, std::get<int64_t>(val.data));
  } else if (std::holds_alternative<std::map<strawbie::IPCKey, strawbie::IPCValue>>(val.data)) {
    sol::table table = lua.create_table();
    auto& map = std::get<std::map<strawbie::IPCKey, strawbie::IPCValue>>(val.data);

    for (auto& [key, value] : map) {
      sol::object lkey;
      if (std::holds_alternative<std::string>(key)) {
        lkey = sol::make_object(lua, std::get<std::string>(key));
      } else if (std::holds_alternative<double>(key)) {
        lkey = sol::make_object(lua, std::get<double>(key));
      } else if (std::holds_alternative<int64_t>(key)) {
        lkey = sol::make_object(lua, std::get<int64_t>(key));
      }

      table[lkey] = deserialise(lua, value);
    }

    return table;
  }

  return sol::lua_nil;
}
