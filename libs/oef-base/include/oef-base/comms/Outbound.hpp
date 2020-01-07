#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/utils/Uri.hpp"

#include <unordered_map>

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

class Outbound : public Endpoint<google::protobuf::Message>
{
public:
  Outbound(const Uri &uri, Core &core, std::size_t sendBufferSize, std::size_t readBufferSize)
    : Endpoint<google::protobuf::Message>(core, sendBufferSize, readBufferSize,
                                          std::unordered_map<std::string, std::string>())
    , uri(uri)
    , core(core)
  {
    this->uri = uri;
  }
  ~Outbound() override = default;

  // run this in a thread.
  bool run();

protected:
  Uri   uri;
  Core &core;

private:
  Outbound(const Outbound &other) = delete;              // { copy(other); }
  Outbound &operator=(const Outbound &other)  = delete;  // { copy(other); return *this; }
  bool      operator==(const Outbound &other) = delete;  // const { return compare(other)==0; }
  bool      operator<(const Outbound &other)  = delete;  // const { return compare(other)==-1; }
};

// namespace std { template<> void swap(Outbound& lhs, Outbound& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const Outbound &output) {}
