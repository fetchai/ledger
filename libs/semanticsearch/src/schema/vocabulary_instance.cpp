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

void VocabularyInstance::Walk(std::function<void(std::string, Vocabulary)> const &callback,
                              std::string const &                                 name)
{
  if (std::type_index(typeid(PropertyMap)) != type_)
  {
    callback(name, shared_from_this());
    return;
  }

  PropertyMap &map = *reinterpret_cast<PropertyMap *>(data_);
  for (auto &o : map)
  {
    o.second->Walk(callback, o.first);
  }
}

VocabularyInstance::Vocabulary &VocabularyInstance::operator[](std::string const &name)
{
  if (std::type_index(typeid(PropertyMap)) != type_)
  {
    throw std::runtime_error(
        "VocabularyInstance index operator error: Vocabulary does not have keys");
  }

  PropertyMap &map = *reinterpret_cast<PropertyMap *>(data_);

  return map[name];
}

void VocabularyInstance::Insert(std::string const &name, Vocabulary const &value)
{
  if (std::type_index(typeid(PropertyMap)) != type_)
  {
    throw std::runtime_error(
        "VocabularyInstance index operator error: Vocabulary does not have keys");
  }

  PropertyMap &map = *reinterpret_cast<PropertyMap *>(data_);
  map[name]        = value;
}

}  // namespace semanticsearch
}  // namespace fetch
