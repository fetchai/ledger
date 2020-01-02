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

#include "semanticsearch/advertisement_register.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

bool AdvertisementRegister::CreateModel(std::string const &name, VocabularySchema const &object)
{
  if (HasModel(name))
  {
    return false;
  }

  return CreateModelInternal(name, object);
}

AdvertisementRegister::SharedModel AdvertisementRegister::GetAdvertisementModel(
    std::string const &name)
{
  assert(model_advertisement_.find(name) != model_advertisement_.end());
  return model_advertisement_[name];
}

void AdvertisementRegister::AdvertiseAgent(AgentId aid, std::string const &name,
                                           SemanticPosition const &position)
{
  assert(HasModel(name));
  auto ad_model = GetAdvertisementModel(name);
  ad_model->SubscribeAgent(aid, position);
}

AdvertisementRegister::AgentIdSet AdvertisementRegister::FindAgents(
    std::string const &name, SemanticPosition const &position, SemanticCoordinateType depth)
{
  auto ad_model = GetAdvertisementModel(name);
  return ad_model->FindAgents(position, depth);
}

AdvertisementRegister::AgentIdSet AdvertisementRegister::FindAgents(std::string const &    name,
                                                                    Vocabulary const &     object,
                                                                    SemanticCoordinateType depth)
{
  auto ad_model = GetAdvertisementModel(name);
  auto position = ad_model->vocabulary_schema()->Reduce(object);

  return ad_model->FindAgents(position, depth);
}

void AdvertisementRegister::OnAddModel(std::string const &name, VocabularySchema const &object)
{
  CreateModelInternal(name, object);
}

bool AdvertisementRegister::CreateModelInternal(std::string const &     name,
                                                VocabularySchema const &object)
{

  SharedModel model          = std::make_shared<VocabularyAdvertisement>(object);
  model_advertisement_[name] = model;

  return true;
}

}  // namespace semanticsearch
}  // namespace fetch
