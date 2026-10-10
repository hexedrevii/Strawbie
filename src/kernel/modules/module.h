#pragma once

namespace strawbie {
  class Module {
  public:
    virtual ~Module() = default;

    virtual void init() = 0;
    virtual void tick() = 0;

    virtual void shutdown() {}
  };
} // namespace strawbie
