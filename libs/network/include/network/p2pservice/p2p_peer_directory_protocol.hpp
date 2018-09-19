#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/p2pservice/p2p_peer_directory.hpp"

namespace fetch {
namespace p2p {

class P2PPeerDirectoryProtocol : public fetch::service::Protocol
{
public:
  enum
  {
    SUGGEST_PEERS      = P2PPeerDirectory::SUGGEST_PEERS,
    NEED_CONNECTIONS   = P2PPeerDirectory::NEED_CONNECTIONS,
    ENOUGH_CONNECTIONS = P2PPeerDirectory::ENOUGH_CONNECTIONS
  };

  P2PPeerDirectoryProtocol(P2PPeerDirectory &directory)
    : directory_(&directory)
  {

    // RPC
    Protocol::ExposeWithClientArg(NEED_CONNECTIONS, directory_, &P2PPeerDirectory::NeedConnections);
    Protocol::ExposeWithClientArg(ENOUGH_CONNECTIONS, directory_,
                                  &P2PPeerDirectory::EnoughConnections);
    Protocol::Expose(SUGGEST_PEERS, directory_, &P2PPeerDirectory::SuggestPeersToConnectTo);

    // Feeds
    Protocol::RegisterFeed(P2PPeerDirectory::FEED_REQUEST_CONNECTIONS, directory_);
    Protocol::RegisterFeed(P2PPeerDirectory::FEED_ENOUGH_CONNECTIONS, directory_);
  }

private:
  P2PPeerDirectory *directory_;
};

}  // namespace p2p
}  // namespace fetch
