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

#include "semanticsearch/module/model_interface_builder.hpp"
#include "semanticsearch/query/query_compiler.hpp"
#include "semanticsearch/query/query_executor.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

ModelIntefaceBuilder::ModelIntefaceBuilder(VocabularySchema model, SemanticSearchModule *factory)
  : model_{std::move(model)}
  , factory_(factory)
{}

ModelIntefaceBuilder::operator bool() const
{
  return model_ != nullptr;
}

ModelIntefaceBuilder &ModelIntefaceBuilder::Field(std::string name, std::string type)
{
  assert(factory_);
  auto field_model = factory_->GetField(type);
  model_->Insert(name, field_model);
  return *this;
}

ModelIntefaceBuilder &ModelIntefaceBuilder::Field(std::string name, ModelIntefaceBuilder proxy)
{
  assert(proxy.model_ != nullptr);
  model_->Insert(name, proxy.model_);
  return *this;
}

ModelIntefaceBuilder &ModelIntefaceBuilder::Field(std::string name, ModelField model)
{
  assert(model != nullptr);
  model_->Insert(name, model);
  return *this;
}

ModelIntefaceBuilder ModelIntefaceBuilder::Vocabulary(std::string name)
{
  auto new_model = PropertiesToSubspace::New();
  model_->Insert(name, new_model);

  return ModelIntefaceBuilder(new_model, factory_);
}

}  // namespace semanticsearch
}  // namespace fetch
