#pragma once

#include "kernel/kernel.hpp"
#include "kernel/process.hpp"
#include "raylib.h"

#include <algorithm>
#include <cstdint>
#include <sol/sol.hpp>

namespace strawbie {
  inline void mkgraphics(sol::state& lua, strawbie::Kernel* kernel, strawbie::Process* proc) {
    sol::table graphics = lua.create_named_table("graphics");

    // Need to know if proc has window???

    graphics["clear"] = [proc](uint8_t r, uint8_t g, uint8_t b) {
      std::fill(proc->video.vram.begin(), proc->video.vram.end(), (Color){r, g, b, 255 });
    };

    graphics["pset"] = [proc](int x, int y, uint8_t r, uint8_t g, uint8_t b) {

    };
  }
}
