#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

namespace fetch {
namespace protocols {

struct ChainKeeperRPC
{
  enum
  {
    PING             = 1,
    HELLO            = 2,
    PUSH_TRANSACTION = 3,
    GET_TRANSACTIONS = 4,
    GET_SUMMARIES    = 5,

    LISTEN_TO                  = 101,
    SET_GROUP_NUMBER           = 102,
    GROUP_NUMBER               = 103,
    COUNT_OUTGOING_CONNECTIONS = 104

  };
};

struct ChainKeeperFeed
{
  enum
  {
    FEED_NEW_BLOCKS = 1,
  };
};
}  // namespace protocols
}  // namespace fetch
