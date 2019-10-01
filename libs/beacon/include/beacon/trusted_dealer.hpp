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

#include "beacon/dkg_output.hpp"
#include "crypto/mcl_dkg.hpp"

namespace fetch {
namespace beacon {

class TrustedDealer
{
public:
  using DkgOutput         = beacon::DkgOutput;
  using MuddleAddress     = byte_array::ConstByteArray;
  using DkgKeyInformation = crypto::mcl::DkgKeyInformation;

  TrustedDealer(std::set<MuddleAddress> cabinet, uint32_t threshold)
    : cabinet_{std::move(cabinet)}
  {
    uint32_t index = 0;
    for (auto const &mem : cabinet_)
    {
      cabinet_index_.insert({mem, index});
      ++index;
    }

    bn::initPairing();
    outputs_ =
        crypto::mcl::TrustedDealerGenerateKeys(static_cast<uint32_t>(cabinet_.size()), threshold);
  }

  DkgOutput GetKeys(MuddleAddress const &address) const
  {
    if (cabinet_index_.find(address) != cabinet_index_.end())
    {
      return DkgOutput(outputs_[cabinet_index_.at(address)], cabinet_);
    }

    return DkgOutput();
  }

private:
  std::set<MuddleAddress>                     cabinet_{};
  std::unordered_map<MuddleAddress, uint32_t> cabinet_index_{};
  std::vector<DkgKeyInformation>              outputs_{};
};
}  // namespace beacon
}  // namespace fetch
