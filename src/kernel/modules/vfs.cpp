#include "kernel/modules/vfs.h"
#include "exceptions.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
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
    if (name == "/" || clean == name || clean.starts_with(name + "/")) {
      if (name.size() > len) {
        len = name.size();
        best = mount;
      }
    }
  }

  return best;
}

std::string strawbie::VirtualFS::read(std::string_view path) {
  std::optional<strawbie::Mount> mount = this->getMount(path);
  if (mount == std::nullopt) {
    throw strawbie::NotMountedException("Directory not mounted.");
  }

  std::string clean = this->normalise(path);
  if (!(mount->permissions & Permissions::READ)) {
    throw strawbie::NoPermissionException("Permission denied: " + clean);
  }

  std::string physical = this->resolve(clean);

  if (!std::filesystem::exists(physical)) {
    throw strawbie::FileException("No such file.");
  }

  if (std::filesystem::is_directory(physical)) {
    throw strawbie::FileException("Is a directory.");
  }

  std::ifstream file(physical);
  if (!file) {
    throw strawbie::FileException("Could not open " + clean);
  }

  const auto size = std::filesystem::file_size(physical);
  std::string content = std::string(size, '\0');

  file.read(content.data(), size);
  file.close();

  return content;
}

std::string strawbie::VirtualFS::normalise(std::string_view path) {
  std::vector<std::string_view> stack;

  size_t start = path.find_first_not_of('/');
  while (start != std::string_view::npos) {
    size_t end = path.find_first_of('/', start);

    std::string_view part = path.substr(start, end - start);

    if (part == ".") {}
    else if (part == "..") {
      if (!stack.empty()) {
        stack.pop_back();
      }
    } else {
      stack.push_back(part);
    }

    start = path.find_first_not_of('/', end);
  }

  if (stack.empty()) return std::string(1, '/');

  std::string joined;
  for (std::string_view part : stack) {
    joined += '/';
    joined += part;
  }

  return joined;
}

std::string strawbie::VirtualFS::resolve(std::string_view path) {
  std::string normalised = this->normalise(path);

  if (!normalised.empty() && normalised.front() == '/') {
    normalised.erase(0, 1);
  }

  return std::format("{}/{}", this->mRoot, normalised);
}
