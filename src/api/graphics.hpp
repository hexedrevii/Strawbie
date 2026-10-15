#pragma once

#include "kernel/kernel.hpp"
#include "kernel/process.hpp"
#include "raylib.h"

#include <cstdint>
#include <mutex>
#include <sol/sol.hpp>

namespace strawbie {
  inline void mkgraphics(sol::state& lua, strawbie::Kernel* kernel, strawbie::Process* proc) {
    sol::table graphics = lua.create_named_table("graphics");

    graphics["window"] = [kernel, proc](int w, int h, std::string title, sol::this_state s) {
      if (proc->video.windowMode) return;
      proc->video.windowMode = true;

      sol::state_view lua(s);

      sol::table payload = lua.create_table();
      payload["w"] = w;
      payload["h"] = h;
      payload["title"] = title;

      proc->video.vram   = GenImageColor(w, h, BLACK);
      proc->video.buffer = GenImageColor(w, h, BLACK);

      unsigned int wm = kernel->findProcess("display");
      if (wm != 0 && proc->pid != wm) kernel->send(wm, "WM_CREATE", payload, proc->pid);
    };

    graphics["clear"] = [proc](int r, int g, int b) {
      if (!proc->video.windowMode) return;

      ImageClearBackground(&proc->video.vram, (Color){(uint8_t)r, (uint8_t)g, (uint8_t)b, 255});
    };

    graphics["pset"] = [proc](int x, int y, int r, int g, int b) {
      if (!proc->video.windowMode) return;

      ImageDrawPixel(&proc->video.vram, x, y, (Color){(uint8_t)r, (uint8_t)g, (uint8_t)b, 255});
    };

    if (proc->priviledges & Priviledges::MASTER) {
      graphics["blit"] = [kernel, proc](unsigned int pid, int dx, int dy) {
        strawbie::Process* source = kernel->getProcess(pid);
        if (!source) return;
        if (!source->video.windowMode) return;

        std::lock_guard<std::mutex> lock(source->video.lock);

        Rectangle srcRec = { 0, 0, (float)source->video.buffer.width, (float)source->video.buffer.height };
        Rectangle destRec = { (float)dx, (float)dy, srcRec.width, srcRec.height };

        ImageDrawImagePro(&proc->video.vram, source->video.buffer, srcRec, destRec, (Vector2){0, 0}, 0, WHITE);
      };
    }
  }
}
