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

#include "semanticsearch/query/query_executor.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

QueryExecutor::QueryExecutor(SharedSemanticSearchModule instance, ErrorTracker &error_tracker)
  : error_tracker_(error_tracker)
  , semantic_search_module_{std::move(instance)}
{}

void QueryExecutor::Execute(Query const &query, Agent agent)
{
  agent_ = std::move(agent);
  error_tracker_.SetSource(query.source, query.filename);
  if (query.statements.empty())
  {
    return;
  }

  for (auto const &stmt : query.statements)
  {
    switch (stmt[0].properties)
    {
    case QueryInstruction::PROP_CTX_MODEL:
      ExecuteDefine(stmt);
      break;
    case QueryInstruction::PROP_CTX_SET:
      ExecuteSet(stmt);
      break;
    case QueryInstruction::PROP_CTX_STORE:
      ExecuteStore(stmt);
      break;
    default:
      error_tracker_.RaiseRuntimeError("Unknown statement type.", stmt[0].token);
      return;
    }

    if (error_tracker_.HasErrors())
    {
      break;
    }
  }
}

QueryExecutor::Vocabulary QueryExecutor::GetInstance(std::string const &name)
{
  assert(context_.Has(name));
  return context_.Get(name);
}

void QueryExecutor::ExecuteStore(CompiledStatement const &stmt)
{
  using Type = QueryInstruction::Type;

  if (stmt[1].type != Type::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable name.", stmt[1].token);
    return;
  }

  auto name = static_cast<std::string>(stmt[1].token);

  if (!context_.Has(name))
  {
    error_tracker_.RaiseSyntaxError("Vairable not defined", stmt[1].token);
    return;
  }

  auto obj        = context_.Get(name);
  auto model_name = context_.GetModelName(name);

  if (!semantic_search_module_->HasModel(model_name))
  {
    error_tracker_.RaiseSyntaxError("Could not find model '" + model_name + "'", stmt[1].token);
    return;
  }

  auto model = semantic_search_module_->GetModel(model_name);
  if (model == nullptr)
  {
    error_tracker_.RaiseInternalError("Model '" + model_name + "' is null.", stmt[1].token);
    return;
  }

  model->VisitSubmodelsWithVocabulary(
      [this, stmt](std::string, std::string mname, Vocabulary obj) {
        auto model = semantic_search_module_->GetModel(mname);
        if (model == nullptr)
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + mname + "'", stmt[1].token);
          return;
        }
        SemanticPosition position = model->Reduce(obj);
        assert(semantic_search_module_->advertisement_register() != nullptr);

        semantic_search_module_->advertisement_register()->AdvertiseAgent(agent_->id, mname,
                                                                          position);
        agent_->RegisterVocabularyLocation(mname, position);
      },
      obj);
}

void QueryExecutor::ExecuteSet(CompiledStatement const &stmt)
{
  using Type    = QueryInstruction::Type;
  std::size_t i = 3;

  if (stmt.size() < i)
  {
    error_tracker_.RaiseSyntaxError("Set statment has incorrect syntax.", stmt[0].token);
    return;
  }

  if (stmt[1].type != Type::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable name.", stmt[1].token);
    return;
  }

  if (stmt[2].type != Type::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable type.", stmt[2].token);
    return;
  }

  std::vector<QueryVariant> stack;
  std::vector<Vocabulary>   scope_objects;
  Vocabulary                last;
  int                       scope_depth = 0;

  QueryVariant object_name =
      NewQueryVariant(static_cast<std::string>(stmt[1].token), TYPE_KEY, stmt[1].token);
  stack.push_back(object_name);

  QueryVariant model_name =
      NewQueryVariant(static_cast<std::string>(stmt[2].token), TYPE_KEY, stmt[2].token);
  stack.push_back(model_name);

  while (i < stmt.size())
  {
    auto x = stmt[i];

    switch (x.type)
    {
    case Type::PUSH_SCOPE:
    {
      ++scope_depth;
      auto obj = VocabularyInstance::New<PropertyMap>({});
      scope_objects.push_back(obj);
      break;
    }
    case Type::POP_SCOPE:
      --scope_depth;
      // TODO(private issue AEA-131): Check enough
      last = scope_objects.back();
      scope_objects.pop_back();

      {
        QueryVariant s =
            NewQueryVariant(last, TYPE_INSTANCE,
                            x.token);  // TODO(private issue AEA-132): Nested shared_ptr
        stack.push_back(s);
      }

      break;

    case Type::ATTRIBUTE:

    {
      auto value = stack.back();
      stack.pop_back();
      auto key = stack.back();
      stack.pop_back();
      auto object = scope_objects.back();
      assert(object != nullptr);

      // TODO(private issue AEA-133): Assert types
      //  model.Field(static_cast<std::string>(key.value), value.model);
      std::string name = static_cast<std::string>(key->As<Token>());
      switch (value->type())
      {
      case TYPE_INTEGER:
        object->Insert(name, VocabularyInstance::New<Int>(value->As<Int>()));
        break;
      case TYPE_FLOAT:
        object->Insert(name, VocabularyInstance::New<Float>(value->As<Float>()));
        break;
      case TYPE_STRING:
        object->Insert(name, VocabularyInstance::New<String>(value->As<String>()));
        break;
      case TYPE_INSTANCE:
        object->Insert(name, value->As<Vocabulary>());
        break;
        // TODO(private issue AEA-134): Create type code index
      default:
        break;
      }
    }

    break;

    case Type::SEPARATOR:
      // TODO(private issue AEA-135): Validate
      break;

    case Type::IDENTIFIER:
    {
      auto         obj = context_.Get(static_cast<std::string>(
          x.token));  // TODO(rivate issue AEA-136): Update context to store query variant
      QueryVariant ele = NewQueryVariant(obj, TYPE_INSTANCE, x.token);

      stack.push_back(ele);
      break;
    }
    case Type::FLOAT:
    {
      QueryVariant ele = NewQueryVariant(Float(x.token.AsFloat()), TYPE_FLOAT, x.token);
      stack.push_back(ele);
      break;
    }
    case Type::INTEGER:
    {
      QueryVariant ele =
          NewQueryVariant(Int(atol(std::string(x.token).c_str())), TYPE_INTEGER, x.token);
      stack.push_back(ele);
      break;
    }
    case Type::STRING:
    {
      std::string  str = static_cast<std::string>(x.token.SubArray(1, x.token.size() - 2));
      QueryVariant ele = NewQueryVariant(str, TYPE_STRING, x.token);
      stack.push_back(ele);
      break;
    }
    case Type::OBJECT_KEY:
    {
      QueryVariant ele = NewQueryVariant(x.token, TYPE_KEY, x.token);
      stack.push_back(ele);
      break;
    }
    default:
      break;
    }

    ++i;
  }

  if (static_cast<bool>(last))
  {

    auto value = stack.back();
    stack.pop_back();
    auto model_var = stack.back();
    stack.pop_back();
    auto key = stack.back();
    stack.pop_back();

    // TODO(private issue AEA-137): add some sanity checks here

    auto name_of_model = model_var->As<std::string>();
    auto model         = semantic_search_module_->GetModel(name_of_model);
    if (model == nullptr)
    {
      error_tracker_.RaiseRuntimeError("Could not find model '" + name_of_model + "'.",
                                       stmt[stmt.size() - 1].token);
      return;
    }

    if (!model->Validate(last))
    {
      error_tracker_.RaiseRuntimeError("Instance does not match model requirements.",
                                       stmt[stmt.size() - 1].token);
      // TODO(private issue AEA-138): List what is wrong
      return;
    }

    context_.Set(key->As<std::string>(), last, name_of_model);
  }
  else
  {
    error_tracker_.RaiseRuntimeError(
        "No object instance was created, only declared. This is not supported yet.",
        stmt[stmt.size() - 1].token);
    return;
  }
}

void QueryExecutor::ExecuteDefine(CompiledStatement const &stmt)
{
  std::size_t i = 1;

  stack_.clear();
  std::vector<ModelInterfaceBuilder> scope_models;
  ModelInterfaceBuilder              last;
  int                                scope_depth = 0;

  using Type = QueryInstruction::Type;

  while (i < stmt.size())
  {
    auto x = stmt[i];

    switch (x.type)
    {
    case Type::PUSH_SCOPE:
      ++scope_depth;
      scope_models.push_back(semantic_search_module_->NewProxy());
      break;

    case Type::POP_SCOPE:

      --scope_depth;
      // TODO(private issue AEA-131): Check enough
      last = scope_models.back();
      scope_models.pop_back();

      {
        QueryVariant s = NewQueryVariant(last.vocabulary_schema(), TYPE_MODEL, x.token);
        stack_.push_back(s);
      }

      break;

    case Type::ATTRIBUTE:
    {
      QueryVariant value = stack_.back();
      stack_.pop_back();
      QueryVariant key = stack_.back();
      stack_.pop_back();

      auto model = scope_models.back();

      // Asserting types
      if (TypeMismatch<ModelField>(value, x.token))
      {
        return;
      }

      if (TypeMismatch<Token>(key, x.token))
      {
        return;
      }

      // Sanity check
      if (value->As<ModelField>() == nullptr)
      {
        error_tracker_.RaiseInternalError("Attribute value is null.", x.token);
        break;
      }

      if (model.vocabulary_schema() == nullptr)
      {
        error_tracker_.RaiseInternalError("Model is null.", x.token);
        break;
      }

      // Doing operation
      model.Field(static_cast<std::string>(key->As<Token>()), value->As<ModelField>());
    }

    break;

    case Type::SEPARATOR:
      // TODO(private issue AEA-135): Validate
      break;

    case Type::IDENTIFIER:
      if (scope_depth == 0)
      {
        QueryVariant ele = NewQueryVariant(x.token, TYPE_KEY, x.token);
        stack_.push_back(ele);
      }
      else
      {
        auto field_name = static_cast<std::string>(x.token);
        if (!semantic_search_module_->HasField(field_name))
        {
          error_tracker_.RaiseRuntimeError(
              "Could not find field: " + static_cast<std::string>(x.token), x.token);
          return;
        }

        QueryVariant ele =
            NewQueryVariant(semantic_search_module_->GetField(field_name), TYPE_MODEL, x.token);
        stack_.push_back(ele);
      }

      break;
    case Type::FLOAT:
    {
      QueryVariant ele = NewQueryVariant(x.token.AsFloat(), TYPE_FLOAT, x.token);
      stack_.push_back(ele);
      break;
    }
    case Type::INTEGER:
    {
      QueryVariant ele =
          NewQueryVariant(Int(atol(std::string(x.token).c_str())), TYPE_INTEGER, x.token);
      stack_.push_back(ele);
      break;
    }
    case Type::STRING:
    {
      std::string  str = static_cast<std::string>(x.token.SubArray(1, x.token.size() - 2));
      QueryVariant ele = NewQueryVariant(str, TYPE_STRING, x.token);
      stack_.push_back(ele);
      break;
    }
    case Type::OBJECT_KEY:
    {
      QueryVariant ele = NewQueryVariant(x.token, TYPE_KEY, x.token);
      stack_.push_back(ele);
      break;
    }
    case Type::FUNCTION:
    {
      QueryVariant ele = NewQueryVariant(x.token, TYPE_FUNCTION_NAME, x.token);
      stack_.push_back(ele);
      break;
    }
    case Type::EXECUTE_CALL:
    {
      if (stack_.empty())
      {
        std::cerr << "INTERNAL ERROR!" << std::endl;  // TODO(private issue AEA-139): Handle this
        exit(-1);
      }

      std::vector<void const *>    args;
      std::vector<std::type_index> arg_signature;

      // Function arguments
      uint64_t n = stack_.size() - 1;
      while ((n != 0) && (stack_[n]->type() != TYPE_FUNCTION_NAME))
      {
        auto arg = stack_[n];
        args.push_back(arg->data());
        arg_signature.push_back(arg->type_index());

        --n;
      }

      // Ordering the function arguments correctly
      std::reverse(args.begin(), args.end());
      std::reverse(arg_signature.begin(), arg_signature.end());

      // Calling
      if (TypeMismatch<Token>(stack_[n], stack_[n]->token()))
      {
        return;
      }
      auto function_name = static_cast<std::string>(stack_[n]->As<Token>());

      if (!semantic_search_module_->HasFunction(function_name))
      {
        error_tracker_.RaiseRuntimeError("Function '" + function_name + "' does not exist.",
                                         stack_[n]->token());
        return;
      }

      auto &          function = (*semantic_search_module_)[function_name];
      std::type_index ret_type = function.return_type();

      if (!function.ValidateSignature(ret_type, arg_signature))
      {
        error_tracker_.RaiseRuntimeError(
            "Call to '" + function_name + "' does not match signature.", stack_[n]->token());
        return;
      }

      QueryVariant ret;
      try
      {
        ret = function(args);
      }
      catch (std::exception const &e)
      {
        error_tracker_.RaiseRuntimeError(e.what(), stack_[n]->token());
        return;
      }

      // Clearing stack
      for (std::size_t j = 0; j < args.size(); ++j)
      {
        stack_.pop_back();
      }
      stack_.pop_back();  // Function name

      // Pushing result
      stack_.push_back(ret);
      break;
    }
    default:
      std::cout << "'" << x.token << "'  - TODO IMPLEMENT "
                << std::endl;  // TODO(private issue AEA-140): Handle this case
      break;
    }

    ++i;
  }

  if (bool(last))
  {
    auto value = stack_.back();
    stack_.pop_back();
    auto key = stack_.back();
    stack_.pop_back();

    // TODO(private issue AEA-137): add some sanity checks here
    semantic_search_module_->AddModel(static_cast<std::string>(key->As<Token>()),
                                      last.vocabulary_schema());
  }
  else
  {
    // TODO(private issue AEA-140): Handle this case
    std::cout << "NOT READY: " << last.vocabulary_schema() << std::endl;
  }
}

}  // namespace semanticsearch
}  // namespace fetch
