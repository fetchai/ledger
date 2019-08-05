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

#include "core/logging.hpp"
#include "dkg/rbc_envelope.hpp"

namespace fetch {
namespace dkg {

constexpr char const *LOGGING_NAME = "DKGMessage";

/**
 * Constructs a RBCMessage from serialised message
 *
 * @return Shared pointer to RBCMessage
 */
std::shared_ptr<RBCMessage> RBCEnvelope::Message() const
{
  RBCSerializer serialiser{payload_};
  switch (type_)
  {
  case MessageType::RBROADCAST:
    return std::make_shared<RBroadcast>(serialiser);
  case MessageType::RECHO:
    return std::make_shared<REcho>(serialiser);
  case MessageType::RREADY:
    return std::make_shared<RReady>(serialiser);
  case MessageType::RREQUEST:
    return std::make_shared<RRequest>(serialiser);
  case MessageType::RANSWER:
    return std::make_shared<RAnswer>(serialiser);
  default:
    FETCH_LOG_ERROR(LOGGING_NAME, "Can not process payload");
    assert(false);
    return nullptr;  // For compiler warnings
  }
}
}  // namespace dkg
}  // namespace fetch
