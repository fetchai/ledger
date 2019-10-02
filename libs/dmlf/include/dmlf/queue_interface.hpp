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

#include <memory>

#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace dmlf {

class QueueInterface
{
public:
  using Bytes = byte_array::ByteArray;

  QueueInterface()          = default;
  virtual ~QueueInterface() = default;

  virtual void        PushNewMessage(Bytes msg) = 0;
  virtual std::size_t size() const              = 0;

  QueueInterface(const QueueInterface &other) = delete;
  QueueInterface &operator=(const QueueInterface &other)  = delete;
  bool            operator==(const QueueInterface &other) = delete;
  bool            operator<(const QueueInterface &other)  = delete;

private:
};

}  // namespace dmlf
}  // namespace fetch
