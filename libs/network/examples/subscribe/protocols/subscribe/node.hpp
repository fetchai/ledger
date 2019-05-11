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

//#include"network/service/service_client.hpp"
#include "network/service/publication_feed.hpp"
#include "protocol.hpp"

namespace fetch {
namespace subscribe {

class Node : public fetch::service::HasPublicationFeed
{
public:
  Node()
  {}
  ~Node()
  {}

  void SendMessage(std::string const &mess)
  {
    std::cout << "Publish " << mess << std::endl;
    this->Publish(fetch::protocols::SubscribeProto::NEW_MESSAGE, mess);
  }
};

}  // namespace subscribe
}  // namespace fetch
