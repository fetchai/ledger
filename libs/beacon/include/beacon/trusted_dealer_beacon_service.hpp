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

#include "beacon/beacon_service.hpp"

namespace fetch {
namespace beacon {

/**
 * Class which allows the beacon service to take in pre-computed DKG public and private keys from a
 * trusted dealer, thereby circumventing the running of the DKG inorder to generate entropy
 */
class TrustedDealerBeaconService : public BeaconService
{
public:
  TrustedDealerBeaconService(MuddleInterface &               muddle,
                             ledger::ManifestCacheInterface &manifest_cache,
                             const CertificatePtr &certificate, SharedEventManager event_manager);

  void StartNewCabinet(CabinetMemberList members, uint32_t threshold, uint64_t round_start,
                       uint64_t round_end, uint64_t start_time, BlockEntropy const &prev_entropy,
                       const DkgOutput &output);
};
}  // namespace beacon
}  // namespace fetch
