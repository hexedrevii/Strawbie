#pragma once

namespace strawbie {
  class Module {
  public:
    virtual ~Module() = default;

    virtual void init() = 0;

    virtual void tick() {}
    virtual void shutdown() {}
  };
} // namespace strawbie
