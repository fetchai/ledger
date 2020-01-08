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

#include "semanticsearch/schema/model_register.hpp"
#include "semanticsearch/vocabular_advertisement.hpp"

namespace fetch {
namespace semanticsearch {

class AdvertisementRegister : public ModelRegister
{
public:
  using Vocabulary          = std::shared_ptr<VocabularyInstance>;
  using SharedModel         = std::shared_ptr<VocabularyAdvertisement>;
  using SharedModelRegister = ModelRegister::SharedModelRegister;
  using Index               = VocabularyAdvertisement::Index;
  using AgentId             = VocabularyAdvertisement::AgentId;
  using AgentIdSet          = VocabularyAdvertisement::AgentIdSet;

  AdvertisementRegister() = default;

  bool        CreateModel(std::string const &name, VocabularySchema const &object);
  SharedModel GetAdvertisementModel(std::string const &name);
  void       AdvertiseAgent(AgentId aid, std::string const &name, SemanticPosition const &position);
  AgentIdSet FindAgents(std::string const &name, SemanticPosition const &position,
                        SemanticCoordinateType depth);
  AgentIdSet FindAgents(std::string const &name, Vocabulary const &object,
                        SemanticCoordinateType depth);

  void OnAddModel(std::string const &name, VocabularySchema const &object) override;

private:
  bool CreateModelInternal(std::string const &name, VocabularySchema const &object);

  std::map<std::string, SharedModel> model_advertisement_;
};

}  // namespace semanticsearch
}  // namespace fetch
