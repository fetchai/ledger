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

#include "beacon/trusted_dealer.hpp"

using fetch::beacon::TrustedDealer;
using DkgOutput = TrustedDealer::DkgOutput;

TrustedDealer::TrustedDealer(std::set<MuddleAddress> cabinet, double threshold)
  : cabinet_{std::move(cabinet)}
  , threshold_{static_cast<uint32_t>(std::floor(threshold * static_cast<double>(cabinet_.size()))) +
               1}
{
  uint32_t index = 0;
  for (auto const &mem : cabinet_)
  {
    cabinet_index_.emplace(mem, index);
    notarisation_units_.emplace_back(new ledger::NotarisationManager);
    notarisation_keys_.insert({mem, notarisation_units_[index]->GenerateKeys()});
    ++index;
  }

  outputs_ =
      crypto::mcl::TrustedDealerGenerateKeys(static_cast<uint32_t>(cabinet_.size()), threshold_);
}

DkgOutput TrustedDealer::GetDkgKeys(MuddleAddress const &address) const
{
  auto it = cabinet_index_.find(address);

  if (it != cabinet_index_.end())
  {
    auto &address_as_index = it->second;
    return DkgOutput(outputs_[address_as_index], cabinet_);
  }

  return DkgOutput();
}

std::pair<TrustedDealer::SharedNotarisationManager, TrustedDealer::CabinetNotarisationKeys>
TrustedDealer::GetNotarisationKeys(MuddleAddress const &address)
{
  auto it = cabinet_index_.find(address);

  if (it != cabinet_index_.end())
  {
    auto &address_as_index = it->second;
    return std::make_pair(notarisation_units_[address_as_index], notarisation_keys_);
  }
  return {nullptr, {}};
}
