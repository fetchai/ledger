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

#include "network/fetch_asio.hpp"
#include <list>
#include <vector>

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

template <typename TXType>
class IMessageWriter
{
public:
  using mutable_buffer       = asio::mutable_buffer;
  using mutable_buffers      = std::vector<mutable_buffer>;
  using consumed_needed_pair = std::pair<std::size_t, std::size_t>;
  using TXQ                  = std::list<TXType>;

  IMessageWriter()          = default;
  virtual ~IMessageWriter() = default;

  virtual consumed_needed_pair initial()
  {
    return consumed_needed_pair(0, 0);
  }

  virtual consumed_needed_pair CheckForSpace(const mutable_buffers &space, TXQ &txq) = 0;

protected:
private:
  IMessageWriter(const IMessageWriter &other) = delete;
  IMessageWriter &operator=(const IMessageWriter &other)  = delete;
  bool            operator==(const IMessageWriter &other) = delete;
  bool            operator<(const IMessageWriter &other)  = delete;
};

// namespace std { template<> void swap(IMessageWriter& lhs, IMessageWriter& rhs) { lhs.swap(rhs); }
// } std::ostream& operator<<(std::ostream& os, const IMessageWriter &output) {}
