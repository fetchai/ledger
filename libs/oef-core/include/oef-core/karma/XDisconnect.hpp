#pragma once

#include <stdexcept>
#include <stdexcept>
#include <string>

class XDisconnect : public std::runtime_error{
public:
  explicit XDisconnect(const std::string &reason)
    : std::runtime_error(std::string("Disconnect because:")+reason)
  {
  }
};
