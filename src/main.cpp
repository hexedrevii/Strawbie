#include "kernel/kernel.hpp"
#include "kernel/modules/vfs.hpp"
#include <raylib.h>

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(420*2, 270*2, "Strawbie");

  SetTargetFPS(60);

  SetWindowMinSize(420, 270);

  // Initialise kernel
  strawbie::Kernel kernel = strawbie::Kernel();
  kernel.module<strawbie::VirtualFS>();

  // Bootup sequence
  kernel.boot();

  while (!WindowShouldClose()) {
    kernel.tick();
    kernel.draw();
  }

  kernel.shutdown();
  CloseWindow();
}
