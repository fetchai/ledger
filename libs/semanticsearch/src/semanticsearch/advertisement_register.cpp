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

#include "semanticsearch/advertisement_register.hpp"

namespace fetch {
namespace semanticsearch {

bool AdvertisementRegister::CreateModel(std::string name, VocabularySchema object)
{
  if (HasModel(name))
  {
    return false;
  }

  return CreateModelInternal(std::move(name), std::move(object));
}

AdvertisementRegister::SharedModel AdvertisementRegister::GetAdvertisementModel(std::string name)
{
  assert(model_advertisement_.find(name) != model_advertisement_.end());
  return model_advertisement_[std::move(name)];
}

void AdvertisementRegister::AdvertiseAgent(AgentId aid, std::string name, SemanticPosition position)
{
  assert(HasModel(name));
  auto ad_model = GetAdvertisementModel(std::move(name));
  ad_model->SubscribeAgent(aid, position);
}

AdvertisementRegister::AgentIdSet AdvertisementRegister::FindAgents(
    std::string name, SemanticPosition position, SemanticCoordinateType granularity)
{
  auto ad_model = GetAdvertisementModel(std::move(name));
  return ad_model->FindAgents(position, granularity);
}

AdvertisementRegister::AgentIdSet AdvertisementRegister::FindAgents(
    std::string name, Vocabulary object, SemanticCoordinateType granularity)
{
  auto ad_model = GetAdvertisementModel(std::move(name));
  auto position = ad_model->model()->Reduce(object);

  return ad_model->FindAgents(position, granularity);
}

void AdvertisementRegister::OnAddModel(std::string name, VocabularySchema object)
{
  CreateModelInternal(std::move(name), std::move(object));
}

bool AdvertisementRegister::CreateModelInternal(std::string name, VocabularySchema object)
{

  SharedModel model          = SharedModel(new VocabularyAdvertisement(object));
  model_advertisement_[name] = model;

  return true;
}

}  // namespace semanticsearch
}  // namespace fetch