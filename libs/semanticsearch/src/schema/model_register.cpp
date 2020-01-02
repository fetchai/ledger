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

#include <cassert>

namespace fetch {
namespace semanticsearch {

void ModelRegister::AddModel(std::string const &name, VocabularySchema const &object)
{
  if (models_.find(name) != models_.end())
  {
    if (object->IsSame(models_[name]))
    {
      return;
    }

    throw std::runtime_error("Model '" + name + "' already exists, but definition mismatch.");
  }

  object->SetModelName(name);
  models_[name] = object;
  OnAddModel(name, object);
}

ModelRegister::VocabularySchema ModelRegister::GetModel(std::string const &name)
{
  if (models_.find(name) == models_.end())
  {
    return nullptr;
  }
  return models_[name];
}

bool ModelRegister::HasModel(std::string const &name)
{
  return models_.find(name) != models_.end();
}

}  // namespace semanticsearch
}  // namespace fetch
