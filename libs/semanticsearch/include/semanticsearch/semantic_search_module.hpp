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
#include <unordered_set>
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
  using KeywordRelation   = std::unordered_map<std::string, std::unordered_set<std::string>>;
  using KeywordProperties = std::unordered_map<std::string, uint64_t>;
  using KeywordTypes      = std::unordered_map<std::string, int32_t>;

  using ConsumerFunction = std::function<int(byte_array::ConstByteArray const &str, uint64_t &pos)>;
  using AllocatorFunction = std::function<QueryVariant(Token const &data)>;
  using SemanticConverterFunction =
      std::function<SemanticCoordinateType(QueryVariant const &value)>;

  struct TypeDetails
  {
    TypeDetails(std::type_index const &t, std::string const &n, ConsumerFunction const &c,
                AllocatorFunction const &a, SemanticConverterFunction const &sc, int32_t d)
      : type{t}
      , name{n}
      , consumer{c}
      , allocator{a}
      , semantic_converter{sc}
      , code{d}
    {}

    std::type_index           type;
    std::string               name;
    ConsumerFunction          consumer;
    AllocatorFunction         allocator;
    SemanticConverterFunction semantic_converter;
    int32_t                   code;
  };

  static ModelIdentifier GenerateID(std::string const &name)
  {

    ModelIdentifier ret;
    ret.scope.address = "ai.fetch";
    ret.scope.version = SemanticCoordinateType(1);
    ret.model_name    = name;

    return ret;
  }

  static SharedSemanticSearchModule New(SharedAdvertisementRegister advertisement_register)
  {
    auto ret =
        SharedSemanticSearchModule(new SemanticSearchModule(std::move(advertisement_register)));

    // Registering primitive types

    ret->RegisterPrimitiveType<ModelField>(GenerateID("ModelField"));

    ret->RegisterPrimitiveType<SemanticCoordinateType>(
        GenerateID("NumberPrimitive"), byte_array::consumers::NumberConsumer<0, 1>,
        [](Token const &token) -> QueryVariant {
          return NewQueryVariant<SemanticCoordinateType>(
              SemanticCoordinateType(static_cast<std::string>(token)), Constants::LITERAL, token);
        },
        [](QueryVariant const &value) -> SemanticCoordinateType {
          if (!value->IsType<SemanticCoordinateType>())
          {
            throw std::runtime_error(
                "Semantic conversion failed: Incorrect type passed to converter.");
          }
          return static_cast<SemanticCoordinateType>(value->As<SemanticCoordinateType>());
        },
        true);

    ret->RegisterPrimitiveType<std::string>(
        GenerateID("StringPrimitive"), byte_array::consumers::StringConsumer<0>,
        [](Token const &token) -> QueryVariant {
          if (token.size() < 2)
          {
            throw std::runtime_error("Incorrectly detected a string.");
          }

          return NewQueryVariant<std::string>(
              static_cast<std::string>(token.SubArray(1, token.size() - 2)), Constants::LITERAL,
              token);
        },
        nullptr, false);

    // Registering reduced types
    ret->RegisterFunction<ModelField, SemanticCoordinateType, SemanticCoordinateType>(
        "Number", [](SemanticCoordinateType from, SemanticCoordinateType to) -> ModelField {
          auto span = static_cast<SemanticCoordinateType>(to - from);
          // Creating an identifier for the reducer
          std::stringstream identifier{""};
          identifier << "Number[" << from << "-" << to << "]";

          // Creating the actually reducer
          SemanticReducer cdr{identifier.str()};

          cdr.SetReducer<SemanticCoordinateType>(1, [span, from](SemanticCoordinateType x) {
            SemanticPosition       ret;
            SemanticCoordinateType multiplier = (SemanticCoordinateType::FP_MAX / span);
            ret.PushBack(static_cast<SemanticCoordinateType>(x - from) * multiplier);

            return ret;
          });

          // And the validator
          cdr.SetValidator<SemanticCoordinateType>(
              [from, to](SemanticCoordinateType x, std::string &error) {
                bool ret = (from <= x) && (x <= to);
                if (!ret)
                {
                  std::stringstream ss{""};
                  ss << "Value not within bounds: " << from << " <= " << x << " <= " << to;
                  error = ss.str();
                  return false;
                }

                return true;
              });

          auto field = TypedSchemaField<SemanticCoordinateType>::New();
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
  void RegisterPrimitiveType(ModelIdentifier const &          name,
                             ConsumerFunction const &         consumer           = nullptr,
                             AllocatorFunction const &        allocator          = nullptr,
                             SemanticConverterFunction const &semantic_converter = nullptr,
                             bool                             hidden             = true)
  {
    SemanticReducer cdr = SemanticReducer{static_cast<std::string>(name) + "DefaultReducer"};

    if (!hidden)
    {
      auto instance = TypedSchemaField<T>::New();
      instance->SetSemanticReducer(std::move(cdr));
      types_[name] = instance;
    }

    assert(type_information_.size() ==
           static_cast<std::size_t>(next_type_code_ - Constants::USER_DEFINED_START));
    type_information_.emplace_back(std::type_index(typeid(T)), name, consumer, allocator,
                                   semantic_converter, next_type_code_++);

    std::type_index idx = std::type_index(typeid(T));
    idx_to_name_[idx]   = name;
  }

  Agent GetAgent(ConstByteArray const &pk)
  {
    return agent_directory_.GetAgent(pk);
  }

  ModelInterfaceBuilder NewModel(ModelIdentifier const &name)
  {
    auto model = ObjectSchemaField::New();
    advertisement_register_->AddModel(name, model);
    types_[name] = model;
    return ModelInterfaceBuilder{model, this};
  }

  ModelInterfaceBuilder NewModel(ModelIdentifier const &name, ModelInterfaceBuilder const &proxy)
  {
    advertisement_register_->AddModel(name, proxy.vocabulary_schema());
    types_[name] = proxy.vocabulary_schema();
    return proxy;
  }

  void AddModel(ModelIdentifier const &name, VocabularySchemaPtr const &object)
  {
    advertisement_register_->AddModel(name, object);
    types_[name] = object;
  }

  ModelInterfaceBuilder NewProxy()
  {
    auto model = ObjectSchemaField::New();
    return ModelInterfaceBuilder{model, this};
  }

  bool HasModel(ModelIdentifier const &name)
  {
    return advertisement_register_->HasModel(name);
  }

  bool HasField(ModelIdentifier const &name)
  {
    return types_.find(name) != types_.end();
  }

  ModelField GetField(ModelIdentifier const &name)
  {
    return types_[name];
  }

  VocabularySchemaPtr GetModel(ModelIdentifier const &name)
  {
    return advertisement_register_->GetModel(name);
  }

  template <typename T>
  ModelIdentifier GetName()
  {
    return GetName(std::type_index(typeid(T)));
  }

  ModelIdentifier GetName(std::type_index idx)
  {

    if (idx_to_name_.find(idx) == idx_to_name_.end())
    {
      ModelIdentifier ret;
      ret.model_name = idx.name();
      return ret;
    }
    return idx_to_name_[idx];
  }

  template <typename R, typename... Args>
  void RegisterFunction(ModelIdentifier const &name, std::function<R(Args...)> function)
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

  TypeDetails const &GetTypeDetails(int32_t const &code)
  {
    auto n = static_cast<std::size_t>(code - Constants::USER_DEFINED_START);

    if (n >= type_information_.size())
    {
      throw std::runtime_error("Tried to fetch non-existent type code.");
    }

    return type_information_[n];
  }

  KeywordRelation const &keyword_relation() const
  {
    return keyword_relation_;
  }

  KeywordProperties const &keyword_properties() const
  {
    return keyword_properties_;
  }

  KeywordTypes const &keyword_type() const
  {
    return keyword_type_;
  }

private:
  explicit SemanticSearchModule(SharedAdvertisementRegister advertisement_register)
    : advertisement_register_(std::move(advertisement_register))
  {}

  std::vector<TypeDetails> type_information_;
  int32_t                  next_type_code_{Constants::USER_DEFINED_START};

  std::unordered_map<std::type_index, ModelIdentifier>            idx_to_name_;
  std::unordered_map<std::type_index, uint64_t>                   idx_to_code_;
  std::unordered_map<std::string, BuiltinQueryFunction::Function> functions_;
  std::map<ModelIdentifier, ModelField>                           types_;

  KeywordRelation keyword_relation_{{"specification", {"version"}},
                                    {"using", {"version"}},
                                    {"find", {"granularity"}},
                                    {"advertise", {"until_block"}},
                                    {"model", {}},
                                    {"instance", {}},
                                    {"vector", {}}};

  KeywordProperties keyword_properties_ = {{"specification", Properties::PROP_IS_OPERATOR},
                                           {"version", Properties::PROP_IS_OPERATOR},
                                           {"using", Properties::PROP_IS_OPERATOR},
                                           {"model", Properties::PROP_CTX_MODEL},
                                           {"advertise", Properties::PROP_IS_OPERATOR},
                                           {"instance", Properties::PROP_IS_OPERATOR},
                                           {"vector", Properties::PROP_IS_OPERATOR},
                                           {"find", Properties::PROP_IS_OPERATOR},
                                           {"until_block", Properties::PROP_IS_OPERATOR},
                                           {"granularity", Properties::PROP_IS_OPERATOR}};

  KeywordTypes keyword_type_ = {{"specification", Constants::SPECIFICATION},
                                {"using", Constants::USING},
                                {"version", Constants::VERSION},
                                {"model", Constants::SET_CONTEXT},
                                {"advertise", Constants::ADVERTISE},
                                {"instance", Constants::STORE},
                                {"vector", Constants::STORE_POSITION},
                                {"find", Constants::SEARCH},
                                {"until_block", Constants::UNTIL},
                                {"granularity", Constants::GRANULARITY}};

  SharedAdvertisementRegister advertisement_register_;
  AgentDirectory              agent_directory_;
};

using SharedSemanticSearchModule = SemanticSearchModule::SharedSemanticSearchModule;

}  // namespace semanticsearch
}  // namespace fetch
