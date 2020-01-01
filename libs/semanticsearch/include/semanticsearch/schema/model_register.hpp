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

#include "semanticsearch/schema/properties_map.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace semanticsearch {

class ModelRegister
{
public:
  using VocabularySchema    = std::shared_ptr<PropertiesToSubspace>;
  using SharedModelRegister = std::shared_ptr<ModelRegister>;

  ModelRegister()          = default;
  virtual ~ModelRegister() = default;

  void             AddModel(std::string const &name, VocabularySchema const &object);
  VocabularySchema GetModel(std::string const &name);
  bool             HasModel(std::string const &name);

  virtual void OnAddModel(std::string const &name, VocabularySchema const &object) = 0;

private:
  std::unordered_map<std::string, VocabularySchema> models_;
};

}  // namespace semanticsearch
}  // namespace fetch
