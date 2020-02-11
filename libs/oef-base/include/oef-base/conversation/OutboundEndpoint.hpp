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

#include <memory>

#include "oef-base/comms/Endpoint.hpp"

class Core;

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

class OutboundEndpoint
{
public:
  OutboundEndpoint()
  {}
  virtual ~OutboundEndpoint()
  {}

  // friend std::ostream& operator<<(std::ostream& os, const OutboundEndpoint &output);
  // friend void swap(OutboundEndpoint &a, OutboundEndpoint &b);
protected:
  // int compare(const OutboundEndpoint &other) const { ... }
  // void copy(const OutboundEndpoint &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(OutboundEndpoint &other) { ... }
private:
  OutboundEndpoint(const OutboundEndpoint &other) = delete;  // { copy(other); }
  OutboundEndpoint &operator                      =(const OutboundEndpoint &other) =
      delete;                                               // { copy(other); return *this; }
  bool operator==(const OutboundEndpoint &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const OutboundEndpoint &other)  = delete;  // const { return compare(other)==-1; }

  // bool operator!=(const OutboundEndpoint &other) const { return compare(other)!=0; }
  // bool operator>(const OutboundEndpoint &other) const { return compare(other)==1; }
  // bool operator<=(const OutboundEndpoint &other) const { return compare(other)!=1; }
  // bool operator>=(const OutboundEndpoint &other) const { return compare(other)!=-1; }
};

// namespace std { template<> void swap(OutboundEndpoint& lhs, OutboundEndpoint& rhs) {
// lhs.swap(rhs); } } std::ostream& operator<<(std::ostream& os, const OutboundEndpoint &output) {}
