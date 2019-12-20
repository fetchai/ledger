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

#include "semanticsearch/schema/abstract_vocabulary_register.hpp"
#include "semanticsearch/vocabular_advertisement.hpp"

namespace fetch {
namespace semanticsearch {

class AdvertisementRegister : public AbstractModelRegister
{
public:
  using ModelInstancePtr              = std::shared_ptr<ModelInstance>;
  using ModelInstanceAdvertisementPtr = std::shared_ptr<ModelInstanceAdvertisement>;
  using AbstractModelRegisterPtr      = AbstractModelRegister::AbstractModelRegisterPtr;
  using Index                         = ModelInstanceAdvertisement::Index;
  using AgentId                       = ModelInstanceAdvertisement::AgentId;
  using AgentIdSetPtr                 = ModelInstanceAdvertisement::AgentIdSetPtr;

  AdvertisementRegister() = default;

  bool CreateModel(SchemaIdentifier const &name, ObjectSchemaFieldPtr const &object);
  ModelInstanceAdvertisementPtr GetAdvertisementModel(SchemaIdentifier const &name);
  void AdvertiseAgent(AgentId aid, SchemaIdentifier const &name, SemanticPosition const &position);
  AgentIdSetPtr FindAgents(SchemaIdentifier const &name, SemanticPosition const &position,
                           DepthParameterType depth);
  AgentIdSetPtr FindAgents(SchemaIdentifier const &name, ModelInstancePtr const &object,
                           DepthParameterType depth);

  void OnAddModel(SchemaIdentifier const &name, ObjectSchemaFieldPtr const &object) override;

private:
  bool CreateModelInternal(SchemaIdentifier const &name, ObjectSchemaFieldPtr const &object);

  std::map<SchemaIdentifier, ModelInstanceAdvertisementPtr> model_advertisement_;
};

}  // namespace semanticsearch
}  // namespace fetch
