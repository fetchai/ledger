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

#include "muddle/address.hpp"
#include "network/uri.hpp"

#include <unordered_map>

namespace fetch {
namespace muddle {

class MuddleInterface;

}  // namespace muddle
namespace ledger {

class LaneController
{
public:
  using Uri        = network::Uri;
  using Address    = muddle::Address;
  using AddressMap = std::unordered_map<Address, Uri>;

  static constexpr char const *LOGGING_NAME = "LaneController";

  // Construction / Destruction
  explicit LaneController(muddle::MuddleInterface &muddle);
  LaneController(LaneController const &) = delete;
  LaneController(LaneController &&)      = delete;
  ~LaneController()                      = default;

  /// Internal controls
  /// @{
  void UseThesePeers(AddressMap const &addresses);
  /// @}

  // Operators
  LaneController &operator=(LaneController const &) = delete;
  LaneController &operator=(LaneController &&) = delete;

private:
  muddle::MuddleInterface &muddle_;
};

}  // namespace ledger
}  // namespace fetch
