
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

#include "network/p2pservice/p2p_identity.hpp"

namespace fetch {
namespace p2p {

class P2PIdentityProtocol : public service::Protocol
{
public:
  enum
  {
    PING             = P2PIdentity::PING,
    HELLO            = P2PIdentity::HELLO,
    UPDATE_DETAILS   = P2PIdentity::UPDATE_DETAILS,
    EXCHANGE_ADDRESS = P2PIdentity::EXCHANGE_ADDRESS
    //    AUTHENTICATE
  };

  P2PIdentityProtocol(P2PIdentity *ctrl)
  {
    this->Expose(PING, ctrl, &P2PIdentity::Ping);
    this->ExposeWithClientArg(HELLO, ctrl, &P2PIdentity::Hello);
    this->ExposeWithClientArg(UPDATE_DETAILS, ctrl, &P2PIdentity::UpdateDetails);
    this->ExposeWithClientArg(EXCHANGE_ADDRESS, ctrl, &P2PIdentity::ExchangeAddress);
  }
};

}  // namespace p2p

}  // namespace fetch
