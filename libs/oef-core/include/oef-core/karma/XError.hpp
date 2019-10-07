#pragma once

#include <stdexcept>
#include <stdexcept>
#include <string>

class XError : public std::runtime_error{
public:
  explicit XError(const std::string &reason)
    : std::runtime_error(reason)
  {
  }
};
