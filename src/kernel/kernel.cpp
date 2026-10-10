#include "kernel/kernel.h"
#include "kernel/modules/module.h"

#include <memory>

void strawbie::Kernel::boot() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->init();
  }
}

void strawbie::Kernel::tick() {
  for (std::unique_ptr<Module>& module : mModules) {
    module->tick();
  }
}
