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

#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

#include <functional>
#include <memory>
#include <string>

namespace fetch {
namespace semanticsearch {

class AbstractSchemaField
{
public:
  using VocabularyInstancePtr = std::shared_ptr<VocabularyInstance>;
  using ModelInterface        = std::shared_ptr<AbstractSchemaField>;
  using FieldVisitor = std::function<void(std::string, std::string, VocabularyInstancePtr)>;

  virtual ~AbstractSchemaField() = default;

  /// Setup
  /// @{
  void SetModelName(std::string name)
  {
    model_name_ = std::move(name);
  }
  /// @}

  /// Used for reduction and validation
  /// @{
  virtual SemanticPosition Reduce(VocabularyInstancePtr const &v)                       = 0;
  virtual bool             Validate(VocabularyInstancePtr const &v, std::string &error) = 0;
  /// @}

  virtual bool VisitFields(FieldVisitor callback, VocabularyInstancePtr instance,
                           std::string name = "")   = 0;
  virtual bool IsSame(ModelInterface const &) const = 0;

  /// Properties
  /// @{
  virtual int             rank() const = 0;
  virtual std::type_index type() const = 0;
  virtual std::string     cdr_uid() const
  {
    return "";
  }

  std::string model_name() const
  {
    return model_name_;
  }
  /// @}

private:
  std::string model_name_;
};

}  // namespace semanticsearch
}  // namespace fetch
