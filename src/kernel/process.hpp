#pragma once

#include "raylib.h"
#include <atomic>
#include <semaphore>
#include <sol/sol.hpp>
#include <thread>
#include <vector>

namespace strawbie {
  enum Priviledges {
    NONE = 0,
    MASTER = 1 << 0
  };

  struct Process {
    unsigned int pid;
    unsigned int parent = 0;

    std::string name;

    float delta;
    uint instructions = 0;
    std::binary_semaphore frame {0};
    std::atomic<bool> alive {true};
    std::jthread thread;

    struct Video {
      std::vector<Color> vram;
      std::vector<Color> buffer;

      std::mutex lock;
    } video;
  };
}
