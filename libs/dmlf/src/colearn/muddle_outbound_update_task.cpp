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

#include "core/byte_array/decoders.hpp"
#include "core/service_ids.hpp"
#include "dmlf/colearn/colearn_protocol.hpp"
#include "dmlf/colearn/muddle_outbound_update_task.hpp"
#include "muddle/rpc/client.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

MuddleOutboundUpdateTask::ExitState MuddleOutboundUpdateTask::run()
{
  auto prom = client_->CallSpecificAddress(
      fetch::byte_array::FromBase64(byte_array::ConstByteArray(target_)), RPC_COLEARN,
      ColearnProtocol::RPC_COLEARN_UPDATE, type_name_, update_, proportion_, random_factor_);
  FETCH_LOG_INFO(LOGGING_NAME, "Sending type=", type_name_, " targ=", target_, " prom=", prom -> id());
  if (prom->Wait())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Success type=", type_name_, " targ=", target_, " prom=", prom -> id());
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Whups type=", type_name_, " targ=", target_, " prom=", prom -> id());
  }
  return ExitState::COMPLETE;
}

bool MuddleOutboundUpdateTask::IsRunnable() const
{
  return true;
}
}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
