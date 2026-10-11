#include "kernel/kernel.h"
#include "kernel/modules/module.h"
#include "kernel/modules/vfs.h"
#include "raylib.h"

#include <format>
#include <memory>
#include <sol/forward.hpp>
#include <sol/protected_function_result.hpp>
#include <sol/sol.hpp>
#include <sol/state_handling.hpp>
#include <stdexcept>

void strawbie::Kernel::boot() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->init();
  }
}

void strawbie::Kernel::tick() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->tick();
  }

  for (auto& sys : this->mProcesses) {
    auto res = sys.second.update();
    if (!res.valid()) {
      sol::error err = res;
    }
  }

  BeginDrawing();
    ClearBackground(WHITE);
    DrawText(std::format("Processes: {}", this->mProcesses.size()).c_str(), 5, 5, 20, BLACK);
    for (auto& sys : this->mProcesses) {
      auto res = sys.second.draw();
      if (!res.valid()) {
        sol::error err = res;
      }
    }
  EndDrawing();
}

unsigned int strawbie::Kernel::process(std::string_view path) {
  strawbie::VirtualFS* vfs = this->get<strawbie::VirtualFS>();
  std::string content = vfs->read(path);

  unsigned int pid = this->mPid;
  this->mPid += 1;

  // TODO: Custom ENV, custom handler, threads, yada yada
  std::unique_ptr<sol::state> lua = std::make_unique<sol::state>();
  lua->open_libraries();

  sol::protected_function_result result = lua->safe_script(content, sol::script_pass_on_error, "@" + vfs->normalise(path));
  if (!result.valid()) {
    sol::error err = result;
    throw std::runtime_error(err.what());
  }

  sol::protected_function init = (*lua)["init"];
  sol::protected_function update = (*lua)["update"];
  sol::protected_function draw = (*lua)["draw"];

  if (init.valid()) {
    auto res = init();
    if (!res.valid()) {
      sol::error err = res;
      throw std::runtime_error(err.what());
    }
  }

  if (update.valid() || draw.valid()) {
    Process proc =(Process){
      .pid=pid,
      .name=vfs->normalise(path),
      .state=std::move(lua),
      .init=init, .update=update, .draw=draw
    };
    this->mProcesses.emplace(pid, std::move(proc));
  }

  return pid;
}
