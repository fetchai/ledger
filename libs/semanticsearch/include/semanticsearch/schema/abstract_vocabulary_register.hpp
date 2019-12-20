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

class AbstractModelRegister
{
public:
  using ObjectSchemaFieldPtr     = std::shared_ptr<ObjectSchemaField>;
  using AbstractModelRegisterPtr = std::shared_ptr<AbstractModelRegister>;

  /// Constructors and destructors
  /// @{
  AbstractModelRegister()          = default;
  virtual ~AbstractModelRegister() = default;
  /// @}

  /// Register methods
  /// @{
  void                 AddModel(ModelIdentifier const &name, ObjectSchemaFieldPtr const &object);
  ObjectSchemaFieldPtr GetModel(ModelIdentifier const &name);
  bool                 HasModel(ModelIdentifier const &name);
  /// @}

  /// Virtual event handlers
  /// @{
  virtual void OnAddModel(ModelIdentifier const &name, ObjectSchemaFieldPtr const &object) = 0;
  /// @}
private:
  std::map<ModelIdentifier, ObjectSchemaFieldPtr> models_;
};

}  // namespace semanticsearch
}  // namespace fetch
