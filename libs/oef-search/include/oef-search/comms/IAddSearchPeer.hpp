#pragma once

#include <string>

class IAddSearchPeer
{
public:

  IAddSearchPeer()                                        = default;
  virtual ~IAddSearchPeer()                               = default;
  IAddSearchPeer(const IAddSearchPeer &other)             = delete;
  IAddSearchPeer &operator=(const IAddSearchPeer &other)  = delete;

  bool            operator==(const IAddSearchPeer &other) = delete;
  bool            operator<(const IAddSearchPeer &other)  = delete;

  virtual void AddPeer(const std::string& peer_uri) = 0;

};
