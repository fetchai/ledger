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

class SematicSearchModule;

class ModelIntefaceBuilder
{
public:
  using ModelField       = std::shared_ptr<VocabularyToSubspaceMapInterface>;
  using VocabularySchema = std::shared_ptr<PropertiesToSubspace>;

  ModelIntefaceBuilder(VocabularySchema model = nullptr, SematicSearchModule *factory = nullptr);
                        operator bool() const;
  ModelIntefaceBuilder &Field(std::string name, std::string type);
  ModelIntefaceBuilder &Field(std::string name, ModelIntefaceBuilder proxy);
  ModelIntefaceBuilder &Field(std::string name, ModelField model);

  ModelIntefaceBuilder Vocabulary(std::string name);

  VocabularySchema model()
  {
    return model_;
  }

private:
  VocabularySchema     model_;
  SematicSearchModule *factory_;
};

}  // namespace semanticsearch
}  // namespace fetch