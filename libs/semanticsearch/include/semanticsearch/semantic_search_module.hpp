#pragma once

#include "semanticsearch/advertisement_register.hpp"
#include "semanticsearch/agent_directory.hpp"
#include "semanticsearch/module/args_resolver.hpp"
#include "semanticsearch/module/builtin_query_function.hpp"
#include "semanticsearch/module/model_interface_builder.hpp"
#include "semanticsearch/query/variant.hpp"
#include "semanticsearch/schema/data_map.hpp"
#include "semanticsearch/schema/semantic_reducer.hpp"

#include <typeindex>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace semanticsearch {

// TODO: Work out which methods are friends of Executor

class SematicSearchModule
{
public:
  using SharedSematicSearchModule   = std::shared_ptr<SematicSearchModule>;
  using SharedAdvertisementRegister = std::shared_ptr<AdvertisementRegister>;
  using ConstByteArray              = fetch::byte_array::ConstByteArray;

  static SharedSematicSearchModule New(SharedAdvertisementRegister advertisement_register)
  {
    return SharedSematicSearchModule(new SematicSearchModule(std::move(advertisement_register)));
  }

  using ModelField       = std::shared_ptr<VocabularyToSubspaceMapInterface>;
  using VocabularySchema = std::shared_ptr<PropertiesToSubspace>;
  using Allocator        = std::function<void()>;
  using AgentId          = AgentDirectory::AgentId;

  template <typename T>
  using Reducer = std::function<SemanticPosition(T const &)>;

  template <typename T>
  void RegisterType(std::string name, bool hidden = false, SemanticReducer cdr = SemanticReducer{})
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

  ModelIntefaceBuilder NewModel(std::string name)
  {
    auto model = PropertiesToSubspace::New();
    advertisement_register_->AddModel(name, model);
    types_[name] = model;
    return ModelIntefaceBuilder{model, this};
  }

  ModelIntefaceBuilder NewModel(std::string name, ModelIntefaceBuilder proxy)
  {
    advertisement_register_->AddModel(name, proxy.model());
    types_[name] = proxy.model();
    return proxy;
  }

  void AddModel(std::string name, VocabularySchema object)
  {
    advertisement_register_->AddModel(name, object);
    types_[name] = object;
  }

  ModelIntefaceBuilder NewProxy()
  {
    auto model = PropertiesToSubspace::New();
    return ModelIntefaceBuilder{model, this};
  }

  bool HasModel(std::string name)
  {
    return advertisement_register_->HasModel(name);
  }

  bool HasField(std::string name)
  {
    return types_.find(name) != types_.end();
  }

  ModelField GetField(std::string name)
  {
    return types_[name];
  }

  VocabularySchema GetModel(std::string name)
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
    return idx_to_name_[std::move(idx)];
  }

  std::string GetName(std::type_index idx)
  {

    if (idx_to_name_.find(idx) == idx_to_name_.end())
    {
      return idx.name();
    }
    return idx_to_name_[std::move(idx)];
  }

  template <typename R, typename... Args>
  void RegisterFunction(std::string name, std::function<R(Args...)> function)
  {
    auto sig         = BuiltinQueryFunction::New<R, Args...>(function);
    functions_[name] = std::move(sig);
  }

  template <typename R, typename... Args, typename F>
  void RegisterFunction(std::string name, F &&lambda)
  {
    RegisterFunction(name, std::function<R(Args...)>(lambda));
  }

  BuiltinQueryFunction &operator[](std::string name)
  {
    return *functions_[std::move(name)];
  }

  QueryVariant Call(std::string name, std::vector<void const *> &args)
  {
    return (*functions_[std::move(name)])(args);
  }

  bool HasFunction(std::string name)
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

private:
  SematicSearchModule(SharedAdvertisementRegister advertisement_register)
    : advertisement_register_(std::move(advertisement_register))
  {}

  std::unordered_map<std::type_index, std::string>                idx_to_name_;
  std::unordered_map<std::string, BuiltinQueryFunction::Function> functions_;
  std::unordered_map<std::string, ModelField>                     types_;
  SharedAdvertisementRegister                                     advertisement_register_;
  AgentDirectory                                                  agent_directory_;
};

using SharedSematicSearchModule = SematicSearchModule::SharedSematicSearchModule;

}  // namespace semanticsearch
}  // namespace fetch