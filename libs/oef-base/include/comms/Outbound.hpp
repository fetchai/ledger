#pragma once

#include "base/src/cpp/comms/Endpoint.hpp"
#include "base/src/cpp/utils/Uri.hpp"

#include <unordered_map>

namespace google
{
  namespace protobuf
  {
    class Message;
  }
}

class Outbound : public Endpoint<google::protobuf::Message>
{
public:
  Outbound(const Uri &uri
           , Core &core
           ,std::size_t sendBufferSize
           ,std::size_t readBufferSize)
    : Endpoint<google::protobuf::Message>(core, sendBufferSize, readBufferSize, std::unordered_map<std::string, std::string>())
    , uri(uri)
    , core(core)
  {
    this -> uri = uri;
  }
  virtual ~Outbound()
  {
  }

  // run this in a thread.
  bool run(void);
protected:
  Uri uri;
  Core &core;
private:
  Outbound(const Outbound &other) = delete; // { copy(other); }
  Outbound &operator=(const Outbound &other) = delete; // { copy(other); return *this; }
  bool operator==(const Outbound &other) = delete; // const { return compare(other)==0; }
  bool operator<(const Outbound &other) = delete; // const { return compare(other)==-1; }


};

//namespace std { template<> void swap(Outbound& lhs, Outbound& rhs) { lhs.swap(rhs); } }
//std::ostream& operator<<(std::ostream& os, const Outbound &output) {}
