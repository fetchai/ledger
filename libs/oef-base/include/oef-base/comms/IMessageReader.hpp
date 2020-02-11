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
#include <vector>

class IMessageReader
{
public:
  using buffer               = asio::const_buffer;
  using buffers              = std::vector<buffer>;
  using consumed_needed_pair = std::pair<std::size_t, std::size_t>;

  IMessageReader()          = default;
  virtual ~IMessageReader() = default;

  virtual consumed_needed_pair initial()
  {
    return consumed_needed_pair(0, 4);
  }

  virtual consumed_needed_pair CheckForMessage(const buffers &data) = 0;

protected:
private:
  IMessageReader(const IMessageReader &other) = delete;
  IMessageReader &operator=(const IMessageReader &other)  = delete;
  bool            operator==(const IMessageReader &other) = delete;
  bool            operator<(const IMessageReader &other)  = delete;
};
