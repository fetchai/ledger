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

#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/utils/Uri.hpp"

class OefSearchEndpoint;
class OutboundConversations;

class SearchTaskFactory : public IOefTaskFactory<OefSearchEndpoint>
{
public:
  SearchTaskFactory(std::shared_ptr<OefSearchEndpoint>     the_endpoint,
                    std::shared_ptr<OutboundConversations> outbounds)
    : IOefTaskFactory(the_endpoint, outbounds)
  {}

  virtual ~SearchTaskFactory()
  {}

  virtual void ProcessMessage(ConstCharArrayBuffer &data)
  {
    current_uri_.diagnostic();
    data.diagnostic();
  }
  // Process the message, throw exceptions if they're bad.

  Uri current_uri_;

protected:
  virtual void EndpointClosed(void)
  {}

private:
  SearchTaskFactory(const SearchTaskFactory &other) = delete;
  SearchTaskFactory &operator=(const SearchTaskFactory &other)  = delete;
  bool               operator==(const SearchTaskFactory &other) = delete;
  bool               operator<(const SearchTaskFactory &other)  = delete;
};
