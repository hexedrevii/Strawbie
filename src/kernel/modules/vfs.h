#include "kernel/modules/module.h"

namespace strawbie {
  class VirtualFS : public Module {
  public:
    void init();
    void tick();
  };
} // namespace strawbie
