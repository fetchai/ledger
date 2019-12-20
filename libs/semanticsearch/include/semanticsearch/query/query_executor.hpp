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

#include "semanticsearch/query/error_tracker.hpp"
#include "semanticsearch/query/execution_context.hpp"
#include "semanticsearch/schema/scope_identifier.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"
#include "semanticsearch/semantic_search_module.hpp"
#include "semanticsearch/vocabular_advertisement.hpp"

namespace fetch {
namespace semanticsearch {

class QueryExecutor
{
public:
  using VocabularySchemaPtr           = SemanticSearchModule::VocabularySchemaPtr;
  using ModelField                    = SemanticSearchModule::ModelField;
  using Token                         = fetch::byte_array::Token;
  using Vocabulary                    = std::shared_ptr<VocabularyInstance>;
  using AbstractVocabularyRegisterPtr = AbstractVocabularyRegister::AbstractVocabularyRegisterPtr;
  using AgentIdSet                    = VocabularyAdvertisement::AgentIdSet;
  using AgentIdSetPtr                 = VocabularyAdvertisement::AgentIdSetPtr;
  using LocalNumbers                  = std::unordered_map<uint64_t, SemanticCoordinateType>;

  QueryExecutor(SharedSemanticSearchModule instance, ErrorTracker &error_tracker);
  AgentIdSetPtr Execute(Query const &query, Agent agent);
  Vocabulary    GetInstance(std::string const &name);

private:
  using PropertyMap = std::map<std::string, std::shared_ptr<VocabularyInstance>>;
  enum
  {
    LOCAL_GRANULARITY,
    LOCAL_BLOCK_EXPIRY,
    LOCAL_VERSION,
    LOCAL_MAX_DEPTH,
    LOCAL_LIMIT
  };

  enum Mode
  {
    SPECIFICATION,
    INSTANTIATION
  };

  SemanticCoordinateType GetLocal(
      uint64_t name, SemanticCoordinateType const &default_value = SemanticCoordinateType{0}) const
  {
    auto it = locals_numbers_.find(name);
    if (it == locals_numbers_.end())
    {
      return default_value;
    }

    return it->second;
  }

  AgentIdSetPtr NewExecute(CompiledStatement const &stmt);
  AgentIdSetPtr ExecuteDefine(CompiledStatement const &stmt);

  template <typename T>
  bool TypeMismatch(QueryVariant const &var, Token token)
  {
    if (!var->IsType<T>())
    {
      auto type_a = semantic_search_module_->GetName<T>();
      auto type_b = semantic_search_module_->GetName(var->type_index());
      error_tracker_.RaiseInternalError("Expected " + static_cast<std::string>(type_a) +
                                            ", but found other type " +
                                            static_cast<std::string>(type_b),
                                        std::move(token));
      return true;
    }
    return false;
  }

  ErrorTracker &             error_tracker_;
  std::vector<QueryVariant>  definition_stack_;
  ExecutionContext           context_;
  SharedSemanticSearchModule semantic_search_module_;

  LocalNumbers    locals_numbers_;
  Mode            mode_{INSTANTIATION};
  ScopeIdentifier scope_;

  uint64_t current_block_time_{0};  //< TODO: create a periodic maintainance to update this
  SemanticCoordinateType default_advertisement_length_{100};
  SemanticCoordinateType default_limit_{10};
  SemanticCoordinateType default_max_depth_{20};
  SemanticCoordinateType default_granularity_{20};

  Agent agent_{nullptr};
};

}  // namespace semanticsearch
}  // namespace fetch
