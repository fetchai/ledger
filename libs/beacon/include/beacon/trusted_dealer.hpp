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

#include "beacon/dkg_output.hpp"
#include "beacon/notarisation_manager.hpp"
#include "crypto/mcl_dkg.hpp"

#include <map>
#include <set>
#include <vector>

namespace fetch {
namespace beacon {

class TrustedDealer
{
public:
  using DkgOutput                 = beacon::DkgOutput;
  using MuddleAddress             = byte_array::ConstByteArray;
  using DkgKeyInformation         = crypto::mcl::DkgKeyInformation;
  using NotarisationManager       = ledger::NotarisationManager;
  using SharedNotarisationManager = std::shared_ptr<NotarisationManager>;
  using CabinetNotarisationKeys   = std::map<MuddleAddress, NotarisationManager::PublicKey>;

  TrustedDealer(std::set<MuddleAddress> cabinet, double threshold);
  DkgOutput GetDkgKeys(MuddleAddress const &address) const;
  std::pair<SharedNotarisationManager, CabinetNotarisationKeys> GetNotarisationKeys(
      MuddleAddress const &address);

private:
  std::set<MuddleAddress>                cabinet_{};
  uint32_t                               threshold_{0};
  std::map<MuddleAddress, uint32_t>      cabinet_index_{};
  std::vector<DkgKeyInformation>         outputs_{};
  std::vector<SharedNotarisationManager> notarisation_units_{};
  CabinetNotarisationKeys                notarisation_keys_{};
};
}  // namespace beacon
}  // namespace fetch
