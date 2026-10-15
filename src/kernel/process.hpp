#pragma once

#include "raylib.h"
#include <atomic>
#include <cstdint>
#include <map>
#include <semaphore>
#include <sol/sol.hpp>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace strawbie {
  enum Priviledges {
    NONE = 0,
    MASTER = 1 << 0
  };

  inline Priviledges operator|(Priviledges a, Priviledges b) {
    return static_cast<Priviledges>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
  }

  struct IPCValue;

  using IPCKey = std::variant<double, int64_t, std::string>;
  using IPCVariant = std::variant<
    std::monostate,
    bool, int64_t, double,
    std::string,
    std::map<IPCKey, IPCValue>
  >;

  struct IPCValue {
    IPCVariant data;
  };

  struct Message {
    unsigned int sender;

    std::string name;
    IPCValue payload;
  };

  struct Process {
    unsigned int pid;
    unsigned int parent = 0;

    std::string name;
    std::string cwd;

    float delta;
    uint instructions = 0;
    std::binary_semaphore frame {0};
    std::atomic<bool> alive {true};
    std::jthread thread;

    struct Video {
      bool windowMode = false;

      Image vram;
      Image buffer;

      std::mutex lock;
    } video;

    std::vector<Message> ipc;
    std::mutex ipcLock;

    Priviledges priviledges;
  };
}
