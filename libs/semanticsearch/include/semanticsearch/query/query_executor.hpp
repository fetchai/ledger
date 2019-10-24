#pragma once

#include "semanticsearch/query/error_tracker.hpp"
#include "semanticsearch/query/execution_context.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"
#include "semanticsearch/semantic_search_module.hpp"

namespace fetch {
namespace semanticsearch {

class QueryExecutor
{
public:
  using VocabularySchema    = SematicSearchModule::VocabularySchema;
  using ModelField          = SematicSearchModule::ModelField;
  using Token               = fetch::byte_array::Token;
  using Vocabulary          = std::shared_ptr<VocabularyInstance>;
  using SharedModelRegister = ModelRegister::SharedModelRegister;

  using Int    = int;  // TODO: Get rid of these
  using Float  = double;
  using String = std::string;

  QueryExecutor(SharedSematicSearchModule instance, ErrorTracker &error_tracker);
  void       Execute(Query const &query, Agent agent);
  Vocabulary GetInstance(std::string name);

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

  // TODO: combine these three into a single execute statement.
  void ExecuteStore(CompiledStatement const &stmt);
  void ExecuteSet(CompiledStatement const &stmt);
  void ExecuteDefine(CompiledStatement const &stmt);

  template <typename T>
  bool TypeMismatch(QueryVariant var, Token token)
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

  ErrorTracker &            error_tracker_;
  std::vector<QueryVariant> stack_;
  ExecutionContext          context_;
  SharedSematicSearchModule semantic_search_module_;
  Agent                     agent_{nullptr};
};

}  // namespace semanticsearch
}  // namespace fetch