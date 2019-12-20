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

#include "semanticsearch/schema/fields/object_schema_field.hpp"
#include "semanticsearch/schema/model_identifier.hpp"

#include <map>
#include <memory>

namespace fetch {
namespace semanticsearch {

class AbstractVocabularyRegister
{
public:
  using VocabularySchemaPtr           = std::shared_ptr<ObjectSchemaField>;
  using AbstractVocabularyRegisterPtr = std::shared_ptr<AbstractVocabularyRegister>;

  /// Constructors and destructors
  /// @{
  AbstractVocabularyRegister()          = default;
  virtual ~AbstractVocabularyRegister() = default;
  /// @}

  /// Register methods
  /// @{
  void                AddModel(ModelIdentifier const &name, VocabularySchemaPtr const &object);
  VocabularySchemaPtr GetModel(ModelIdentifier const &name);
  bool                HasModel(ModelIdentifier const &name);
  /// @}

  /// Virtual event handlers
  /// @{
  virtual void OnAddModel(ModelIdentifier const &name, VocabularySchemaPtr const &object) = 0;
  /// @}
private:
  std::map<ModelIdentifier, VocabularySchemaPtr> models_;
};

}  // namespace semanticsearch
}  // namespace fetch
