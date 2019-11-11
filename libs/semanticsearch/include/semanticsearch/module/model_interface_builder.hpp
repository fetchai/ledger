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

#include "semanticsearch/schema/abstract_schema_field.hpp"
#include "semanticsearch/schema/object_schema_field.hpp"

#include <memory>

namespace fetch {
namespace semanticsearch {

class SemanticSearchModule;

class ModelInterfaceBuilder
{
public:
  using ModelField          = std::shared_ptr<AbstractSchemaField>;
  using VocabularySchemaPtr = std::shared_ptr<ObjectSchemaField>;

  explicit ModelInterfaceBuilder(VocabularySchemaPtr   model   = nullptr,
                                 SemanticSearchModule *factory = nullptr);

  explicit               operator bool() const;
  ModelInterfaceBuilder &Field(std::string const &name, std::string const &type);
  ModelInterfaceBuilder &Field(std::string const &name, ModelInterfaceBuilder proxy);
  ModelInterfaceBuilder &Field(std::string const &name, ModelField const &model);

  ModelInterfaceBuilder Vocabulary(std::string const &name);

  VocabularySchemaPtr const &vocabulary_schema() const
  {
    return model_;
  }

private:
  VocabularySchemaPtr   model_;
  SemanticSearchModule *factory_;
};

}  // namespace semanticsearch
}  // namespace fetch
