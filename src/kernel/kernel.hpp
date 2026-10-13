#pragma once

#include "kernel/modules/module.hpp"
#include "kernel/process.hpp"
#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace strawbie {
  class Kernel {
  public:
    template <typename T, typename... Args>
    void module(Args &&...args) {
      std::unique_ptr<Module> module = std::make_unique<T>(std::forward<Args>(args)...);
      mModules.push_back(std::move(module));

      mServices[typeid(T).hash_code()] = mModules.back().get();
    }

    template <typename T>
    T* get() {
      return static_cast<T*>(mServices[typeid(T).hash_code()]);
    }

    void boot();
    void tick();
    void shutdown();

    unsigned int process(std::string_view path, Priviledges priviledges = Priviledges::NONE);

  private:
    void kworker(Process* process, const std::string content);

    std::vector<std::unique_ptr<Module>> mModules;
    std::unordered_map<size_t, Module*> mServices;

    unsigned int mWindowManager;

    unsigned int mPid = 1;
    std::map<unsigned int, std::unique_ptr<Process>> mProcesses;
  };
} // namespace strawbie
