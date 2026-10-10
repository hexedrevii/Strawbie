#include <raylib.h>

int main() {
  InitWindow(800, 600, "Strawbie");

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(WHITE);
    DrawText("Hello, World!", 5, 5, 20, BLACK);
    EndDrawing();
  }

  CloseWindow();
}
