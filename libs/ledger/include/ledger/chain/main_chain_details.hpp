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

#include <atomic>

namespace fetch {
namespace chain {

struct MainChainDetails
{
  MainChainDetails() : is_controller(false), is_peer(false), is_miner(false), is_outgoing(false) {}

  std::atomic<bool> is_controller;
  std::atomic<bool> is_peer;
  std::atomic<bool> is_miner;
  std::atomic<bool> is_outgoing;
};

}  // namespace chain
}  // namespace fetch
