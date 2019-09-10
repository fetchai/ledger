#pragma once

#include <stdexcept>
#include <stdexcept>
#include <string>

class XKarma : public std::runtime_error{
public:
  explicit XKarma(const std::string &action)
    : std::runtime_error(std::string("Karma limit reached attempting:")+action)
  {
  }
};
