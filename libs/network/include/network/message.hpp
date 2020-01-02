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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <deque>
#include <functional>

namespace fetch {
namespace network {
using MessageBuffer = byte_array::ByteArray;
struct MessageType
{
  using Callback = std::function<void()>;

  MessageBuffer buffer;
  Callback      success{nullptr};
  Callback      failure{nullptr};
};
using MessageQueueType = std::deque<MessageType>;
}  // namespace network
}  // namespace fetch
