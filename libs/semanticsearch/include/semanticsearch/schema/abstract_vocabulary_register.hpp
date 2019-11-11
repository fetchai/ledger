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

#include "semanticsearch/schema/object_schema_field.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace semanticsearch {

class AbstractVocabularyRegister
{
public:
  using VocabularySchemaPtr              = std::shared_ptr<ObjectSchemaField>;
  using SharedAbstractVocabularyRegister = std::shared_ptr<AbstractVocabularyRegister>;

  /// Constructors and destructors
  /// @{
  AbstractVocabularyRegister()          = default;
  virtual ~AbstractVocabularyRegister() = default;
  /// @}

  /// Register methods
  /// @{
  void                AddModel(std::string const &name, VocabularySchemaPtr const &object);
  VocabularySchemaPtr GetModel(std::string const &name);
  bool                HasModel(std::string const &name);
  /// @}

  /// Virtual event handlers
  /// @{
  virtual void OnAddModel(std::string const &name, VocabularySchemaPtr const &object) = 0;
  /// @}
private:
  std::unordered_map<std::string, VocabularySchemaPtr> models_;
};

}  // namespace semanticsearch
}  // namespace fetch
