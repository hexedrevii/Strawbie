#include "kernel/modules/vfs.h"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <format>
#include <vector>

void strawbie::VirtualFS::init() {
  this->mount("/", Permissions::READ | Permissions::WRITE | Permissions::EXEC);
}

void strawbie::VirtualFS::mount(std::string_view path, strawbie::Permissions perms) {
  std::string clean = this->normalise(path);

  this->mMounts.emplace_back(clean, perms);
}

std::optional<strawbie::Mount> strawbie::VirtualFS::getMount(std::string_view path) {
  std::string clean = this->normalise(path);

  std::optional<strawbie::Mount> best = std::nullopt;
  size_t len = 0;

  for (strawbie::Mount& mount : this->mMounts) {
    const std::string& name = mount.path;
    if (clean == name || clean.starts_with(name + "/")) {
      if (name.size() > len) {
        len = name.size();
        best = mount;
      }
    }
  }

  return best;
}

std::string strawbie::VirtualFS::normalise(std::string_view path) {
  std::vector<std::string_view> stack;

  size_t start = path.find_first_not_of(this->mSeparator);
  while (start != std::string_view::npos) {
    size_t end = path.find_first_of(this->mSeparator, start);

    std::string_view part = path.substr(start, end - start);

    if (part == ".") {}
    else if (part == "..") {
      if (!stack.empty()) {
        stack.pop_back();
      }
    } else {
      stack.push_back(part);
    }

    start = path.find_first_not_of(this->mSeparator, end);
  }

  if (stack.empty()) return std::string(1, this->mSeparator);

  std::string joined;
  for (std::string_view part : stack) {
    joined += this->mSeparator;
    joined += part;
  }

  return joined;
}

std::string strawbie::VirtualFS::resolve(std::string_view path) {
  std::string normalised = this->normalise(path);

  // This sorta means nothing
  // mSeparator is forced to be / anyway due to limitations of the real OS.
  // but whatever it looks cool
  std::replace(normalised.begin(), normalised.end(), this->mSeparator, '/');
  if (!normalised.empty() && normalised.front() == '/') {
    normalised.erase(0, 1);
  }

  return std::format("{}/{}", this->mRoot, normalised);
}
