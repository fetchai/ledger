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

class AdvertisementRegister : public AbstractVocabularyRegister
{
public:
  using Vocabulary                    = std::shared_ptr<VocabularyInstance>;
  using SharedModel                   = std::shared_ptr<VocabularyAdvertisement>;
  using AbstractVocabularyRegisterPtr = AbstractVocabularyRegister::AbstractVocabularyRegisterPtr;
  using Index                         = VocabularyAdvertisement::Index;
  using AgentId                       = VocabularyAdvertisement::AgentId;
  using AgentIdSetPtr                 = VocabularyAdvertisement::AgentIdSetPtr;

  AdvertisementRegister() = default;

  bool        CreateModel(ModelIdentifier const &name, VocabularySchemaPtr const &object);
  SharedModel GetAdvertisementModel(ModelIdentifier const &name);
  void AdvertiseAgent(AgentId aid, ModelIdentifier const &name, SemanticPosition const &position);
  AgentIdSetPtr FindAgents(ModelIdentifier const &name, SemanticPosition const &position,
                           DepthParameterType depth);
  AgentIdSetPtr FindAgents(ModelIdentifier const &name, Vocabulary const &object,
                           DepthParameterType depth);

  void OnAddModel(ModelIdentifier const &name, VocabularySchemaPtr const &object) override;

private:
  bool CreateModelInternal(ModelIdentifier const &name, VocabularySchemaPtr const &object);

  std::map<ModelIdentifier, SharedModel> model_advertisement_;
};

}  // namespace semanticsearch
}  // namespace fetch
