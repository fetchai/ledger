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

#include "beacon/trusted_dealer.hpp"

using fetch::beacon::TrustedDealer;
using DkgOutput = TrustedDealer::DkgOutput;

TrustedDealer::TrustedDealer(std::set<MuddleAddress> cabinet, uint32_t threshold)
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

DkgOutput TrustedDealer::GetKeys(MuddleAddress const &address) const
{
  if (cabinet_index_.find(address) != cabinet_index_.end())
  {
    return DkgOutput(outputs_[cabinet_index_.at(address)], cabinet_);
  }

  return DkgOutput();
}
