#include "kernel/kernel.h"
#include "kernel/modules/vfs.h"
#include <raylib.h>

int main() {
  InitWindow(800, 600, "Strawbie");

  // Initialise kernel
  strawbie::Kernel kernel = strawbie::Kernel();
  kernel.module<strawbie::VirtualFS>();

  // Bootup sequence
  kernel.boot();

  // test
  kernel.process("/hello.lua");
  kernel.process("/long.lua");

  while (!WindowShouldClose()) {
    kernel.tick();
  }

  kernel.shutdown();
  CloseWindow();
}
