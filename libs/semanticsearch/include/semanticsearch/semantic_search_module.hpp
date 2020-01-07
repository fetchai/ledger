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

#include "semanticsearch/advertisement_register.hpp"
#include "semanticsearch/agent_directory.hpp"
#include "semanticsearch/module/args_resolver.hpp"
#include "semanticsearch/module/builtin_query_function.hpp"
#include "semanticsearch/module/model_interface_builder.hpp"
#include "semanticsearch/query/abstract_query_variant.hpp"
#include "semanticsearch/schema/data_map.hpp"
#include "semanticsearch/schema/semantic_reducer.hpp"

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
  using ConstByteArray              = fetch::byte_array::ConstByteArray;

  static SharedSemanticSearchModule New(SharedAdvertisementRegister advertisement_register)
  {
    return SharedSemanticSearchModule(new SemanticSearchModule(std::move(advertisement_register)));
  }

  using ModelField       = std::shared_ptr<VocabularyToSubspaceMapInterface>;
  using VocabularySchema = std::shared_ptr<PropertiesToSubspace>;
  using Allocator        = std::function<void()>;
  using AgentId          = AgentDirectory::AgentId;

  template <typename T>
  using Reducer = std::function<SemanticPosition(T const &)>;

  template <typename T>
  void RegisterType(std::string const &name, bool hidden = false,
                    SemanticReducer cdr = SemanticReducer{})
  {
    if (!hidden)
    {
      auto instance = DataToSubspaceMap<T>::New();
      instance->SetSemanticReducer(std::move(cdr));
      types_[name] = instance;
    }
    std::type_index idx = std::type_index(typeid(T));
    idx_to_name_[idx]   = name;
  }

  Agent GetAgent(ConstByteArray const &pk)
  {
    return agent_directory_.GetAgent(pk);
  }

  ModelInterfaceBuilder NewModel(std::string const &name)
  {
    auto model = PropertiesToSubspace::New();
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

  void AddModel(std::string const &name, VocabularySchema const &object)
  {
    advertisement_register_->AddModel(name, object);
    types_[name] = object;
  }

  ModelInterfaceBuilder NewProxy()
  {
    auto model = PropertiesToSubspace::New();
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

  VocabularySchema GetModel(std::string const &name)
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

private:
  explicit SemanticSearchModule(SharedAdvertisementRegister advertisement_register)
    : advertisement_register_(std::move(advertisement_register))
  {}

  std::unordered_map<std::type_index, std::string>                idx_to_name_;
  std::unordered_map<std::string, BuiltinQueryFunction::Function> functions_;
  std::unordered_map<std::string, ModelField>                     types_;
  SharedAdvertisementRegister                                     advertisement_register_;
  AgentDirectory                                                  agent_directory_;
};

using SharedSemanticSearchModule = SemanticSearchModule::SharedSemanticSearchModule;

}  // namespace semanticsearch
}  // namespace fetch
