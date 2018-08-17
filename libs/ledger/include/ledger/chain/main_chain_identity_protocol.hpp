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

namespace fetch {
namespace chain {

class MainChainIdentityProtocol : public service::Protocol
{
public:
  enum
  {
    PING = 1,
    HELLO,
    AUTHENTICATE_CONTROLLER

  };

  MainChainIdentityProtocol(MainChainIdentity *ctrl)
  {
    this->Expose(PING, ctrl, &MainChainIdentity::Ping);
    this->Expose(HELLO, ctrl, &MainChainIdentity::Hello);
    this->Expose(AUTHENTICATE_CONTROLLER, ctrl, &MainChainIdentity::AuthenticateController);
  }
};

}  // namespace chain

}  // namespace fetch
