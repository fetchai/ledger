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

#include "beacon/beacon_setup_service.hpp"
#include "beacon/trusted_dealer.hpp"

namespace fetch {
namespace beacon {

/**
 * Class which allows the beacon service to take in pre-computed DKG public and private keys from a
 * trusted dealer, thereby circumventing the running of the DKG inorder to generate entropy
 */
class TrustedDealerSetupService : public BeaconSetupService
{
public:
  static constexpr char const *LOGGING_NAME = "TrustedDealerSetupService";

  using CertificatePtr            = BeaconSetupService::CertificatePtr;
  using CabinetMemberList         = BeaconSetupService::CabinetMemberList;
  using CallbackFunction          = BeaconSetupService::CallbackFunction;
  using SharedNotarisationManager = TrustedDealer::SharedNotarisationManager;
  using CabinetNotarisationKeys   = TrustedDealer::CabinetNotarisationKeys;

  TrustedDealerSetupService(MuddleInterface &muddle, ManifestCacheInterface &manifest_cache,
                            CertificatePtr const &certificate, double threshold,
                            uint64_t aeon_period);

  void StartNewCabinet(
      CabinetMemberList members, uint64_t round_start, uint64_t start_time,
      BlockEntropy const &prev_entropy, DkgOutput const &output,
      std::pair<SharedNotarisationManager, CabinetNotarisationKeys> notarisation_keys = {nullptr,
                                                                                         {}});

private:
  using SharedAeonExecutionUnit = BeaconSetupService::SharedAeonExecutionUnit;

  CertificatePtr certificate_;
  double         threshold_;
  uint64_t       aeon_period_;
};
}  // namespace beacon
}  // namespace fetch
