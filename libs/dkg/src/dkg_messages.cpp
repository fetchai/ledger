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

#include "dkg/dkg_messages.hpp"
#include "logging/logging.hpp"

namespace fetch {
namespace dkg {

constexpr char const *LOGGING_NAME = "DKGMessage";

/**
 * Getter funcion for the serialised message contained inside the
 * DKGEnvelope
 *
 * @return Shared pointer to deserialised message
 */
std::shared_ptr<DKGMessage> DKGEnvelope::Message() const
{
  DKGSerializer serialiser{serialisedMessage_};
  switch (type_)
  {
  case MessageType::CONNECTIONS:
    return std::make_shared<ConnectionsMessage>(serialiser);
  case MessageType::COEFFICIENT:
    return std::make_shared<CoefficientsMessage>(serialiser);
  case MessageType::SHARE:
    return std::make_shared<SharesMessage>(serialiser);
  case MessageType::COMPLAINT:
    return std::make_shared<ComplaintsMessage>(serialiser);
  case MessageType::NOTARISATION_KEY:
    return std::make_shared<NotarisationKeyMessage>(serialiser);
  case MessageType::FINAL_STATE:
    return std::make_shared<FinalStateMessage>(serialiser);
  default:
    FETCH_LOG_ERROR(LOGGING_NAME, "Cannot process payload");
    assert(false);
    return nullptr;  // For compiler warnings
  }
}

}  // namespace dkg
}  // namespace fetch
