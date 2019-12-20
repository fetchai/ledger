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
#include "semanticsearch/schema/fields/abstract_schema_field.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

#include <functional>
#include <map>
#include <memory>

namespace fetch {
namespace semanticsearch {
class ObjectSchemaField : public AbstractSchemaField
{
public:
  using Vocabulary = std::shared_ptr<ModelInstance>;
  using Type       = std::map<std::string, Vocabulary>;

  using ModelInterface = std::shared_ptr<AbstractSchemaField>;
  using FieldModel     = std::shared_ptr<ObjectSchemaField>;
  using ModelMap       = std::map<std::string, ModelInterface>;
  using FieldVisitor   = std::function<void(std::string, std::string, Vocabulary)>;
  ObjectSchemaField(ObjectSchemaField const &) = delete;
  ObjectSchemaField &operator=(ObjectSchemaField const &) = delete;

  static FieldModel New(ModelMap m = {})
  {
    FieldModel ret = FieldModel(new ObjectSchemaField(std::move(m)));

    return ret;
  }

  SemanticPosition Reduce(Vocabulary const &v) override
  {
    SemanticPosition ret{};
    if (std::type_index(typeid(Type)) != v->type())
    {
      throw std::runtime_error("Reducer does not match schema type.");
    }

    auto const &data = *reinterpret_cast<Type *>(v->data_);
    if (array_.size() != data.size())
    {
      throw std::runtime_error("Array is incorrect size.");
    }

    auto it1 = array_.begin();
    auto it2 = data.begin();

    for (std::size_t i = 0; i < data.size(); ++i)
    {
      auto entries = it1->second->Reduce(it2->second);
      for (auto &e : entries)
      {
        ret.PushBack(e);
      }
      ++it1, ++it2;
    }

    return ret;
  }

  bool Validate(Vocabulary const &v, std::string &error) override
  {
    if (std::type_index(typeid(Type)) != v->type())
    {
      error = "Type mismatch.";
      return false;
    }

    auto const &data = *reinterpret_cast<Type *>(v->data_);

    if (array_.size() != data.size())
    {
      error = "Size mismatch.";  // TODO: Could be more informative
      return false;
    }

    auto it1 = array_.begin();
    auto it2 = data.begin();

    for (std::size_t i = 0; i < data.size(); ++i)
    {
      if (it1->first != it2->first)
      {
        error = "Type mismatch of key: " + it1->first + " vs  " + it2->first;
        return false;
      }

      if (!it1->second->Validate(it2->second, error))
      {
        return false;
      }
      ++it1, ++it2;
    }

    return true;
  }

  int rank() const override
  {
    return rank_;
  }

  bool VisitFields(FieldVisitor callback, Vocabulary obj, std::string name = "") override
  {

    if (type() != obj->type())
    {
      return false;
    }

    auto const &data = *reinterpret_cast<Type *>(obj->data_);

    if (array_.size() != data.size())
    {
      return false;
    }

    auto it1 = array_.begin();
    auto it2 = data.begin();

    callback(name, this->model_name(), obj);

    for (std::size_t i = 0; i < data.size(); ++i)
    {
      if (it1->first != it2->first)
      {
        return false;
      }

      if (!it1->second->VisitFields(callback, it2->second, it1->first))
      {
        return false;
      }
      ++it1, ++it2;
    }

    return true;
  }

  void Insert(std::string const &name, ModelInterface const &model)
  {
    array_[name] = model;
    rank_ += model->rank();
  }

  std::type_index type() const override
  {
    return {typeid(Type)};
  }

  bool IsSame(ModelInterface const &optr) const override
  {
    if (type() != optr->type())
    {
      return false;
    }

    ObjectSchemaField const &other = *reinterpret_cast<ObjectSchemaField const *>(optr.get());

    if (array_.size() != other.array_.size())
    {
      return false;
    }

    auto it1 = array_.begin();
    auto it2 = other.array_.begin();
    for (std::size_t i = 0; i < array_.size(); ++i)
    {
      if (it1->first != it2->first)
      {
        return false;
      }

      if (!it1->second->IsSame(it2->second))
      {
        return false;
      }
      ++it1, ++it2;
    }

    return true;
  }

private:
  explicit ObjectSchemaField(ModelMap m)
    : array_(std::move(m))
  {
    rank_ = 0;
    for (auto &a : array_)
    {
      rank_ += a.second->rank();
    }
  }

  ModelMap array_;
  int      rank_ = 0;
};

}  // namespace semanticsearch
}  // namespace fetch
