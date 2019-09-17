#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
  /// @{
  Outbound(Uri const &uri, Core &core, std::size_t sendBufferSize, std::size_t readBufferSize)
    : Endpoint<google::protobuf::Message>(core, sendBufferSize, readBufferSize,
                                          std::unordered_map<std::string, std::string>())
    , uri(uri)
    , core(core)
  {
    this->uri = uri;
  }
  Outbound(Outbound const &other) = delete;
  virtual ~Outbound()             = default;
  /// @}

  /// @{
  Outbound &operator=(Outbound const &other)  = delete;
  bool      operator==(Outbound const &other) = delete;
  bool      operator<(Outbound const &other)  = delete;
  ///@}

  // run this in a thread.
  bool run();

protected:
  // TODO: Follow style
  Uri   uri;
  Core &core;
};
