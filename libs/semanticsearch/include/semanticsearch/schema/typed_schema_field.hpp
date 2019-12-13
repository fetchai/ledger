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
#include "semanticsearch/schema/abstract_schema_field.hpp"
#include "semanticsearch/schema/semantic_reducer.hpp"
#include <functional>

namespace fetch {
namespace semanticsearch {

template <typename T>
class TypedSchemaField : public AbstractSchemaField
{
public:
  using Type         = T;
  using FieldModel   = std::shared_ptr<TypedSchemaField>;
  using Variant      = std::shared_ptr<VocabularyInstance>;
  using FieldVisitor = std::function<void(std::string, std::string, VocabularyInstancePtr)>;

  /// Constructors and destructors
  /// @{
  static FieldModel New();
  TypedSchemaField()                         = delete;
  TypedSchemaField(TypedSchemaField const &) = delete;
  TypedSchemaField &operator=(TypedSchemaField const &) = delete;
  ~TypedSchemaField() override                          = default;
  /// @}

  /// Traversal and validation
  /// @{
  SemanticPosition Reduce(VocabularyInstancePtr const &v) override;
  bool             Validate(VocabularyInstancePtr const &v, std::string &error) override;
  bool             IsSame(ModelInterface const &optr) const override;
  bool             VisitFields(FieldVisitor callback, VocabularyInstancePtr obj,
                               std::string name = "") override;
  /// @}

  /// Properties
  /// @{
  int              rank() const override;
  std::type_index  type() const override;
  SemanticReducer &constrained_data_reducer();
  /// @}

  /// Setters
  /// @{
  void SetSemanticReducer(SemanticReducer r);
  /// @}
private:
  explicit TypedSchemaField(std::type_index type)
    : type_{type}
  {}

  SemanticReducer constrained_data_reducer_{""};
  std::type_index type_;
  int             rank_{0};
};

template <typename T>
typename TypedSchemaField<T>::FieldModel TypedSchemaField<T>::New()
{
  FieldModel ret = FieldModel(new TypedSchemaField<T>(std::type_index(typeid(T))));
  return ret;
}

template <typename T>
SemanticPosition TypedSchemaField<T>::Reduce(VocabularyInstancePtr const &v)
{
  if (std::type_index(typeid(T)) != type_)
  {
    throw std::runtime_error("Reducer does not match schema type.");
  }

  return constrained_data_reducer_.Reduce(v->data_);
}

template <typename T>
bool TypedSchemaField<T>::Validate(VocabularyInstancePtr const &v, std::string &error)
{
  if (type_ != v->type())
  {
    error = "Type mismatch.";
    return false;
  }

  return constrained_data_reducer_.Validate(v->data_, error);
}

template <typename T>
int TypedSchemaField<T>::rank() const
{
  return constrained_data_reducer_.rank();
}

template <typename T>
std::type_index TypedSchemaField<T>::type() const
{
  return type_;
}

template <typename T>
bool TypedSchemaField<T>::IsSame(ModelInterface const &optr) const
{
  return (type() == optr->type());
  // TODO(private issue AEA-130): Check reducers and validators - they are currently ignored.
}

template <typename T>
bool TypedSchemaField<T>::VisitFields(FieldVisitor /*callback*/, VocabularyInstancePtr /*obj*/,
                                      std::string /*name*/)
{
  // A typed field does not have any sub fields, hence we just
  // return
  return true;
}

template <typename T>
void TypedSchemaField<T>::SetSemanticReducer(SemanticReducer r)
{
  constrained_data_reducer_ = std::move(r);
}

template <typename T>
SemanticReducer &TypedSchemaField<T>::constrained_data_reducer()
{
  return constrained_data_reducer_;
}

}  // namespace semanticsearch
}  // namespace fetch
