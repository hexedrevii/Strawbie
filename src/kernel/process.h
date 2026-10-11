#pragma once

#include <memory>
#include <sol/sol.hpp>

namespace strawbie {
  struct Process {
    unsigned int pid;
    unsigned int parent = 0;

    std::string name;

    std::unique_ptr<sol::state> state;
    sol::protected_function init;
    sol::protected_function update;
    sol::protected_function draw;
  };
}
