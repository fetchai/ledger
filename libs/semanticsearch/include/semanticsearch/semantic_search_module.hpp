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

#include "semanticsearch/advertisement_register.hpp"
#include "semanticsearch/agent_directory.hpp"
#include "semanticsearch/module/arguments_to_type_vector.hpp"
#include "semanticsearch/module/builtin_query_function.hpp"
#include "semanticsearch/module/model_interface_builder.hpp"
#include "semanticsearch/module/vector_to_arguments.hpp"
#include "semanticsearch/query/abstract_query_variant.hpp"
#include "semanticsearch/schema/fields/typed_schema_field.hpp"
#include "semanticsearch/schema/semantic_reducer.hpp"
#include "semanticsearch/semantic_constants.hpp"

#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace semanticsearch {

// TODO(private issue AEA-141): Work out which methods are friends of Executor

class SemanticSearchModule
{
public:
  using SharedSemanticSearchModule  = std::shared_ptr<SemanticSearchModule>;
  using SharedAdvertisementRegister = std::shared_ptr<AdvertisementRegister>;
  using ConstByteArray              = byte_array::ConstByteArray;
  using Token                       = byte_array::Token;

  using ConsumerFunction = std::function<int(byte_array::ConstByteArray const &str, uint64_t &pos)>;
  using AllocatorFunction = std::function<QueryVariant(Token const &data)>;

  struct TypeDetails
  {
    TypeDetails(std::type_index const &t, std::string const &n, ConsumerFunction const &c,
                AllocatorFunction const &a, int32_t d)
      : type{t}
      , name{n}
      , consumer{c}
      , allocator{a}
      , code{d}
    {}

    std::type_index   type;
    std::string       name;
    ConsumerFunction  consumer;
    AllocatorFunction allocator;
    int32_t           code;
  };

  static SharedSemanticSearchModule New(SharedAdvertisementRegister advertisement_register)
  {
    auto ret =
        SharedSemanticSearchModule(new SemanticSearchModule(std::move(advertisement_register)));

    // Registering primitive types
    ret->RegisterPrimitiveType<ModelField>("ModelField");
    ret->RegisterPrimitiveType<int64_t>(
        "Integer", byte_array::consumers::NumberConsumer<0, 1>,
        [](Token const &token) -> QueryVariant {
          return NewQueryVariant<int64_t>(static_cast<int64_t>(token.AsInt()), Constants::LITERAL,
                                          token);
        },
        true);

    ret->RegisterPrimitiveType<uint64_t>("UnsignedInteger");
    ret->RegisterPrimitiveType<std::string>(
        "String", byte_array::consumers::StringConsumer<0>,
        [](Token const &token) -> QueryVariant {
          //   TODO: Throw is size does not fit
          return NewQueryVariant<std::string>(
              static_cast<std::string>(token.SubArray(1, token.size() - 2)), Constants::LITERAL,
              token);
        },
        false);

    // Registering reduced types
    ret->RegisterFunction<ModelField, int64_t, int64_t>(
        "BoundedInteger", [](int64_t from, int64_t to) -> ModelField {
          auto            span = static_cast<uint64_t>(to - from);
          SemanticReducer cdr{"BoundedIntegerReducer[" + std::to_string(from) + "-" +
                              std::to_string(to) + "]"};

          cdr.SetReducer<int64_t>(1, [span, from](int64_t x) {
            SemanticPosition ret;
            uint64_t         multiplier = uint64_t(-1) / span;
            ret.push_back(static_cast<uint64_t>(x + from) * multiplier);

            return ret;
          });

          cdr.SetValidator<int64_t>([from, to](int64_t x, std::string &error) {
            bool ret = (from <= x) && (x <= to);
            if (!ret)
            {
              error = "Value not within bouds: " + std::to_string(from) +
                      " <=" + std::to_string(x) + "<=" + std::to_string(to);
              return false;
            }

            return true;
          });

          auto field = TypedSchemaField<int64_t>::New();
          field->SetSemanticReducer(cdr);

          return field;
        });

    return ret;
  }

  using ModelField          = std::shared_ptr<AbstractSchemaField>;
  using VocabularySchemaPtr = std::shared_ptr<ObjectSchemaField>;
  using Allocator           = std::function<void()>;
  using AgentId             = AgentDirectory::AgentId;

  template <typename T>
  using Reducer = std::function<SemanticPosition(T const &)>;

  template <typename T>
  void RegisterPrimitiveType(
      std::string const &name, ConsumerFunction const &consumer = nullptr,
      AllocatorFunction const &allocator = nullptr,
      bool                     hidden = true)  // TODO: Add over loaded type that sets reducer uid
  {
    SemanticReducer cdr = SemanticReducer{name + "DefaultReducer"};

    if (!hidden)
    {
      auto instance = TypedSchemaField<T>::New();
      instance->SetSemanticReducer(std::move(cdr));
      types_[name] = instance;
    }

    type_information_.emplace_back(std::type_index(typeid(T)), name, consumer, allocator,
                                   ++next_type_code_);
    std::type_index idx = std::type_index(typeid(T));
    idx_to_name_[idx]   = name;
  }

  Agent GetAgent(ConstByteArray const &pk)
  {
    return agent_directory_.GetAgent(pk);
  }

  ModelInterfaceBuilder NewModel(std::string const &name)
  {
    auto model = ObjectSchemaField::New();
    advertisement_register_->AddModel(name, model);
    types_[name] = model;
    return ModelInterfaceBuilder{model, this};
  }

  ModelInterfaceBuilder NewModel(std::string const &name, ModelInterfaceBuilder const &proxy)
  {
    advertisement_register_->AddModel(name, proxy.vocabulary_schema());
    types_[name] = proxy.vocabulary_schema();
    return proxy;
  }

  void AddModel(std::string const &name, VocabularySchemaPtr const &object)
  {
    advertisement_register_->AddModel(name, object);
    types_[name] = object;
  }

  ModelInterfaceBuilder NewProxy()
  {
    auto model = ObjectSchemaField::New();
    return ModelInterfaceBuilder{model, this};
  }

  bool HasModel(std::string const &name)
  {
    return advertisement_register_->HasModel(name);
  }

  bool HasField(std::string const &name)
  {
    return types_.find(name) != types_.end();
  }

  ModelField GetField(std::string const &name)
  {
    return types_[name];
  }

  VocabularySchemaPtr GetModel(std::string const &name)
  {
    return advertisement_register_->GetModel(name);
  }

  template <typename T>
  std::string GetName()
  {

    std::type_index idx = std::type_index(typeid(T));
    if (idx_to_name_.find(idx) == idx_to_name_.end())
    {
      return idx.name();
    }
    return idx_to_name_[idx];
  }

  std::string GetName(std::type_index idx)
  {

    if (idx_to_name_.find(idx) == idx_to_name_.end())
    {
      return idx.name();
    }
    return idx_to_name_[idx];
  }

  template <typename R, typename... Args>
  void RegisterFunction(std::string const &name, std::function<R(Args...)> function)
  {
    auto sig         = BuiltinQueryFunction::New<R, Args...>(function);
    functions_[name] = std::move(sig);
  }

  template <typename R, typename... Args, typename F>
  void RegisterFunction(std::string const &name, F &&lambda)
  {
    RegisterFunction(name, std::function<R(Args...)>(lambda));
  }

  BuiltinQueryFunction &operator[](std::string const &name)
  {
    return *functions_[name];
  }

  QueryVariant Call(std::string const &name, std::vector<void const *> &args)
  {
    return (*functions_[name])(args);
  }

  bool HasFunction(std::string const &name)
  {
    return functions_.find(name) != functions_.end();
  }

  SharedAdvertisementRegister advertisement_register() const
  {
    return advertisement_register_;
  }

  AgentId RegisterAgent(ConstByteArray const &pk)
  {
    return agent_directory_.RegisterAgent(pk);
  }

  bool UnregisterAgent(ConstByteArray const &pk)
  {
    return agent_directory_.UnregisterAgent(pk);
  }

  std::vector<TypeDetails> const &type_information() const
  {
    return type_information_;
  }

private:
  explicit SemanticSearchModule(SharedAdvertisementRegister advertisement_register)
    : advertisement_register_(std::move(advertisement_register))
  {}

  std::vector<TypeDetails> type_information_;
  int32_t                  next_type_code_{Constants::USER_DEFINED_START};

  std::unordered_map<std::type_index, std::string>                idx_to_name_;
  std::unordered_map<std::type_index, uint64_t>                   idx_to_code_;
  std::unordered_map<std::string, BuiltinQueryFunction::Function> functions_;
  std::unordered_map<std::string, ModelField>                     types_;
  SharedAdvertisementRegister                                     advertisement_register_;
  AgentDirectory                                                  agent_directory_;
};

using SharedSemanticSearchModule = SemanticSearchModule::SharedSemanticSearchModule;

}  // namespace semanticsearch
}  // namespace fetch
