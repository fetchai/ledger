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

#include "semanticsearch/schema/properties_map.hpp"
#include "semanticsearch/schema/subspace_map_interface.hpp"

#include <memory>

namespace fetch {
namespace semanticsearch {

class SemanticSearchModule;

class ModelInterfaceBuilder
{
public:
  using ModelField       = std::shared_ptr<VocabularyToSubspaceMapInterface>;
  using VocabularySchema = std::shared_ptr<PropertiesToSubspace>;

  ModelInterfaceBuilder(VocabularySchema model = nullptr, SemanticSearchModule *factory = nullptr);
                         operator bool() const;
  ModelInterfaceBuilder &Field(std::string name, std::string type);
  ModelInterfaceBuilder &Field(std::string name, ModelInterfaceBuilder proxy);
  ModelInterfaceBuilder &Field(std::string name, ModelField model);

  ModelInterfaceBuilder Vocabulary(std::string name);

  VocabularySchema model()
  {
    return model_;
  }

private:
  VocabularySchema      model_;
  SemanticSearchModule *factory_;
};

}  // namespace semanticsearch
}  // namespace fetch
