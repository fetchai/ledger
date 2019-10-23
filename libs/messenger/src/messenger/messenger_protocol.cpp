#include "messenger/messenger_protocol.hpp"
#include "messenger/messenger_api.hpp"

namespace fetch {
namespace messenger {

MessengerProtocol::MessengerProtocol(MessengerAPI *api)
  : Protocol()
{
  this->ExposeWithClientContext(REGISTER_MESSENGER, api, &MessengerAPI::RegisterMessenger);
  this->ExposeWithClientContext(UNREGISTER_MESSENGER, api, &MessengerAPI::UnregisterMessenger);

  this->ExposeWithClientContext(SEND_MESSAGE, api, &MessengerAPI::SendMessage);
  this->ExposeWithClientContext(GET_MESSAGES, api, &MessengerAPI::GetMessages);

  this->ExposeWithClientContext(FIND_MESSENGERS, api, &MessengerAPI::FindMessengers);
  this->ExposeWithClientContext(ADVERTISE, api, &MessengerAPI::Advertise);
}

}  // namespace messenger
}  // namespace fetch