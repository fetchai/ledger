#pragma once

#include "logging/logging.hpp"
#include <iostream>
#include <string>

class Uri
{
public:
  static constexpr char const *LOGGING_NAME = "Uri";

  Uri(const std::string &s)
  {
    this->s = s;
    parse(s);
  }

  Uri() = default;

  Uri(const Uri &other)
  {
    copy(other);
  }
  Uri &operator=(const Uri &other)
  {
    copy(other);
    return *this;
  }

  virtual ~Uri()
  {}

  std::string s;
  std::string proto;
  std::string host;
  std::string path;
  uint32_t    port;
  bool        valid;

  void diagnostic() const
  {
    FETCH_LOG_INFO(LOGGING_NAME, "valid=", valid, " proto=\"", proto, "\" host=\"", host,
                   "\" port=", port, " path=\"", path, "\"");
  }

  inline std::string GetSocketAddress() const
  {
    return proto + "://" + host + ":" + std::to_string(port);
  }

  inline std::string ToString() const
  {
    return GetSocketAddress() + "/" + path;
  }

protected:
  void parse(const std::string &s);

  void copy(const Uri &other)
  {
    s     = other.s;
    valid = other.valid;
    proto = other.proto;
    host  = other.host;
    port  = other.port;
    path  = other.path;
  }

private:
  bool operator==(const Uri &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const Uri &other)  = delete;  // const { return compare(other)==-1; }
};
