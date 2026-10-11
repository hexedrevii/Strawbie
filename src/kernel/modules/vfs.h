#include "kernel/modules/module.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace strawbie {
  enum Permissions {
    READ  = 1 << 0,
    WRITE = 1 << 1,
    EXEC  = 1 << 2,
  };

  inline Permissions operator|(Permissions a, Permissions b) {
    return static_cast<Permissions>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
  }

  struct Mount {
    std::string path;
    Permissions permissions;
  };

  class VirtualFS : public Module {
  private:
    // Where root begins in the real filesystem
    const std::string mRoot = "rootfs";

    std::vector<Mount> mMounts;
  public:
    void init();

    void mount(std::string_view path, Permissions perms);
    std::optional<Mount> getMount(std::string_view path);

    std::string read(std::string_view path);

    std::string normalise(std::string_view path);
    std::string resolve(std::string_view path);
  };
} // namespace strawbie
