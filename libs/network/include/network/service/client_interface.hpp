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

#include "core/serializers/counter.hpp"
#include "core/serializers/serializable_exception.hpp"
#include "network/message.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/message_types.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "network/service/types.hpp"

#include <cstdint>
#include <cstdio>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace service {

class ServiceClientInterface
{
public:
  static constexpr char const *LOGGING_NAME = "ServiceClientInterface";

  // Construction / Destruction
  ServiceClientInterface()          = default;
  virtual ~ServiceClientInterface() = default;

protected:
  using CallId           = uint64_t;
  using CallIdPromiseMap = std::unordered_map<CallId, Promise>;
  using PromiseMap       = std::unordered_map<PromiseCounter, Promise>;

  bool ProcessServerMessage(network::MessageBuffer const &msg);
  void ProcessRPCResult(network::MessageBuffer const &msg, service::SerializerType &params);

  // Pending promise issues
  void    AddPromise(Promise const &promise);
  Promise ExtractPromise(PromiseCounter id);
  void    RemovePromise(PromiseCounter id);

  PromiseMap promises_;
  Mutex      promises_mutex_;
};
}  // namespace service
}  // namespace fetch
