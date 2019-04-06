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

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/uri.hpp"

using Uri     = fetch::network::Uri;
using Muddle  = fetch::muddle::Muddle;
using Server  = fetch::muddle::rpc::Server;
using Client  = fetch::muddle::rpc::Client;
using Address = Muddle::Address;  // == a crypto::Identity.identifier_

using MuddlePtr = std::shared_ptr<Muddle>;
using ServerPtr = std::shared_ptr<Server>;
using ClientPtr = std::shared_ptr<Client>;

const int SERVICE_TEST = 1;
const int CHANNEL_RPC  = 1;
