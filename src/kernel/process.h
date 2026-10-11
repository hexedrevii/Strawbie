#pragma once

#include <atomic>
#include <semaphore>
#include <sol/sol.hpp>
#include <thread>

namespace strawbie {
  struct Process {
    unsigned int pid;
    unsigned int parent = 0;

    std::string name;

    float delta;
    uint instructions = 0;
    std::binary_semaphore frame {0};
    std::atomic<bool> alive {true};
    std::jthread thread;
  };
}
