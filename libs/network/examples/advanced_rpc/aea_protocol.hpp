#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "aea_functionality.hpp"
#include "commands.hpp"

#include "network/service/server.hpp"

class AEAProtocol : public AEAFunctionality, public fetch::service::Protocol
{
public:
  AEAProtocol(std::string const &info) : AEAFunctionality(info), fetch::service::Protocol()
  {

    using namespace fetch::service;
    /*
    auto get_info =  new CallableClassMember<AEAFunctionality,
    std::string()>(this, &AEAFunctionality::get_info); auto connect =  new
    CallableClassMember<AEAFunctionality, void(std::string, uint16_t)>(this,
    &AEAFunctionality::Connect);

    */
    AEAFunctionality *controller = (AEAFunctionality *)this;
    this->Expose(AEACommands::GET_INFO, controller, &AEAFunctionality::get_info);
    this->Expose(AEACommands::CONNECT, controller, &AEAFunctionality::Connect);
  }
};
