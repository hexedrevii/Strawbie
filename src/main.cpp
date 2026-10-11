#include "kernel/kernel.h"
#include "kernel/modules/vfs.h"
#include <iostream>
#include <optional>
#include <raylib.h>

int main() {
  InitWindow(800, 600, "Strawbie");

  // Initialise kernel
  strawbie::Kernel kernel = strawbie::Kernel();
  kernel.module<strawbie::VirtualFS>();

  // Bootup sequence
  kernel.boot();

  // -- Test
  strawbie::VirtualFS* vfs = kernel.get<strawbie::VirtualFS>();

  std::optional<strawbie::Mount> root = vfs->getMount("/");
  if (root == std::nullopt)
    std::cout << "Root not mounted.\n";
  else
    std::cout << "Root mounted.\n";

  std::optional<strawbie::Mount> sys = vfs->getMount("/sys");
  if (root == std::nullopt)
    std::cout << "Sys not mounted.\n";
  else
    std::cout << "Sys mounted.\n";
  // -- Test

  while (!WindowShouldClose()) {
    kernel.tick();

    BeginDrawing();
    ClearBackground(WHITE);
    EndDrawing();
  }

  CloseWindow();
}
