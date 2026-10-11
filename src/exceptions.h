#pragma once

#include <stdexcept>

namespace strawbie {
  class NotMountedException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  class NoPermissionException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

  class FileException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };
}
