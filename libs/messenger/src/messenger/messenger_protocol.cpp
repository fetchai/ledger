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

#include "messenger/messenger_api.hpp"
#include "messenger/messenger_protocol.hpp"

namespace fetch {
namespace messenger {

MessengerProtocol::MessengerProtocol(MessengerAPI *api)
{
  this->ExposeWithClientContext(REGISTER_MESSENGER, api, &MessengerAPI::RegisterMessenger);
  this->ExposeWithClientContext(UNREGISTER_MESSENGER, api, &MessengerAPI::UnregisterMessenger);

  this->ExposeWithClientContext(SEND_MESSAGE, api, &MessengerAPI::SendMessage);
  this->ExposeWithClientContext(GET_MESSAGES, api, &MessengerAPI::GetMessages);
  this->ExposeWithClientContext(CLEAR_MESSAGES, api, &MessengerAPI::ClearMessages);

  this->ExposeWithClientContext(FIND_MESSENGERS, api, &MessengerAPI::FindAgents);
  this->ExposeWithClientContext(ADVERTISE, api, &MessengerAPI::Advertise);
}

}  // namespace messenger
}  // namespace fetch
