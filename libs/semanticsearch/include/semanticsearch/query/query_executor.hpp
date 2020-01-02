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

#include "semanticsearch/query/error_tracker.hpp"
#include "semanticsearch/query/execution_context.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"
#include "semanticsearch/semantic_search_module.hpp"

namespace fetch {
namespace semanticsearch {

class QueryExecutor
{
public:
  using VocabularySchema    = SemanticSearchModule::VocabularySchema;
  using ModelField          = SemanticSearchModule::ModelField;
  using Token               = fetch::byte_array::Token;
  using Vocabulary          = std::shared_ptr<VocabularyInstance>;
  using SharedModelRegister = ModelRegister::SharedModelRegister;

  using Int    = int;  // TODO(private issue AEA-126): Get rid of these
  using Float  = double;
  using String = std::string;

  QueryExecutor(SharedSemanticSearchModule instance, ErrorTracker &error_tracker);
  void       Execute(Query const &query, Agent agent);
  Vocabulary GetInstance(std::string const &name);

private:
  using PropertyMap = std::map<std::string, std::shared_ptr<VocabularyInstance>>;

  enum
  {
    TYPE_NONE  = 0,
    TYPE_MODEL = 10,
    TYPE_INSTANCE,
    TYPE_KEY,
    TYPE_STRING,
    TYPE_INTEGER,
    TYPE_FLOAT,

    TYPE_FUNCTION_NAME
  };

  // TODO(private issue AEA-128): combine these three into a single execute statement.
  void ExecuteStore(CompiledStatement const &stmt);
  void ExecuteSet(CompiledStatement const &stmt);
  void ExecuteDefine(CompiledStatement const &stmt);

  template <typename T>
  bool TypeMismatch(QueryVariant const &var, Token token)
  {
    if (!var->IsType<T>())
    {
      auto type_a = semantic_search_module_->GetName<T>();
      auto type_b = semantic_search_module_->GetName(var->type_index());
      error_tracker_.RaiseInternalError("Expected " + type_a + ", but found other type " + type_b,
                                        std::move(token));
      return true;
    }
    return false;
  }

  ErrorTracker &             error_tracker_;
  std::vector<QueryVariant>  stack_;
  ExecutionContext           context_;
  SharedSemanticSearchModule semantic_search_module_;
  Agent                      agent_{nullptr};
};

}  // namespace semanticsearch
}  // namespace fetch
