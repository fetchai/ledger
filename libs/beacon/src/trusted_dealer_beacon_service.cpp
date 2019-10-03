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

#include "beacon/trusted_dealer_beacon_service.hpp"

namespace fetch {
namespace beacon {

TrustedDealerBeaconService::TrustedDealerBeaconService(
    MuddleInterface &muddle, ledger::ManifestCacheInterface &manifest_cache,
    const CertificatePtr &certificate, SharedEventManager event_manager)
  : BeaconService{muddle, manifest_cache, certificate, std::move(event_manager)}
{}

void TrustedDealerBeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold,
                                                 uint64_t round_start, uint64_t round_end,
                                                 uint64_t            start_time,
                                                 BlockEntropy const &prev_entropy,
                                                 const DkgOutput &   output)
{
  auto diff_time = int64_t(static_cast<uint64_t>(std::time(nullptr))) - int64_t(start_time);
  FETCH_LOG_INFO(LOGGING_NAME, "Starting new cabinet from ", round_start, " to ", round_end,
                 " at time: ", start_time, " (diff): ", diff_time);

  // Check threshold meets the requirements for the RBC
  uint32_t rbc_threshold{0};
  if (members.size() % 3 == 0)
  {
    rbc_threshold = static_cast<uint32_t>(members.size() / 3 - 1);
  }
  else
  {
    rbc_threshold = static_cast<uint32_t>(members.size() / 3);
  }
  if (threshold < rbc_threshold)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Threshold is below RBC threshold. Reset to rbc threshold");
    threshold = rbc_threshold;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

  beacon->manager.SetCertificate(certificate_);
  beacon->manager.NewCabinet(members, threshold);
  beacon->manager.SetDkgOutput(output);

  // Setting the aeon details
  beacon->aeon.round_start               = round_start;
  beacon->aeon.round_end                 = round_end;
  beacon->aeon.members                   = std::move(members);
  beacon->aeon.start_reference_timepoint = start_time;
  beacon->aeon.block_entropy_previous    = prev_entropy;

  aeon_exe_queue_.push_back(beacon);
}
}  // namespace beacon
}  // namespace fetch
