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

#include "semanticsearch/query/query_executor.hpp"
#include "semanticsearch/semantic_constants.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

QueryExecutor::QueryExecutor(SharedSemanticSearchModule instance, ErrorTracker &error_tracker)
  : error_tracker_(error_tracker)
  , semantic_search_module_{std::move(instance)}
{}

QueryExecutor::AgentIdSet QueryExecutor::Execute(Query const &query, Agent agent)
{
  agent_ = std::move(agent);

  // Checking that the agent acutally exists
  if (agent_ == nullptr)
  {
    // Setting error occurance to very beginning of file
    Token zero;
    zero.SetLine(0);
    zero.SetChar(0);
    error_tracker_.RaiseRuntimeError("Agent not found. Did you remember to register it?", zero);
    return nullptr;
  }

  // Preparing error tracker
  error_tracker_.SetSource(query.source, query.filename);

  // Stopping if there is nothing to execute
  if (query.statements.empty())
  {
    return nullptr;
  }

  // Executing each statement in program
  AgentIdSet ret{nullptr};
  for (auto const &stmt : query.statements)
  {

    switch (stmt[0].properties)
    {
    case Properties::PROP_CTX_MODEL:
      ret = ExecuteDefine(stmt);
      break;

    case Properties::PROP_CTX_ADVERTISE:
      ret = ExecuteStore(stmt);
      break;
    case Properties::PROP_CTX_FIND:
      ret = ExecuteFind(stmt);
      break;
    default:
      ret = NewExecute(stmt);
      //      error_tracker_.RaiseRuntimeError("Unknown statement type.", stmt[0].token);
      //      return nullptr;
      break;
    }

    // Breaking at first encountered error.
    if (error_tracker_.HasErrors())
    {
      return nullptr;
    }
  }
  return ret;
}

QueryExecutor::AgentIdSet QueryExecutor::NewExecute(CompiledStatement const &stmt)
{
  AgentIdSet result;

  std::size_t i = 0;

  std::vector<QueryVariant> stack;
  std::vector<Vocabulary>   scope_objects;

  Vocabulary last;
  int        scope_depth = 0;

  try
  {
    while (i < stmt.size())
    {
      auto x = stmt[i];

      switch (x.type)
      {
      case Constants::PUSH_SCOPE:
      {
        ++scope_depth;

        // Creating new property map to hold the contents
        auto obj = VocabularyInstance::New<PropertyMap>({});
        scope_objects.push_back(obj);
        break;
      }
      case Constants::POP_SCOPE:
        if (scope_objects.empty())
        {
          error_tracker_.RaiseInternalError("Cannot pop off empty stack for object creation.",
                                            x.token);
          return nullptr;
        }

        --scope_depth;

        last = scope_objects.back();
        scope_objects.pop_back();

        {
          // TODO(private issue AEA-132): Nested shared_ptr - consider copying ?
          QueryVariant s = NewQueryVariant(last, Constants::TYPE_INSTANCE, x.token);
          stack.push_back(s);
        }

        break;

      case Constants::ATTRIBUTE:
      {
        auto value = stack.back();
        stack.pop_back();
        auto key = stack.back();
        stack.pop_back();

        auto object = scope_objects.back();

        // Sanity check on object
        if (object == nullptr)
        {
          error_tracker_.RaiseInternalError("Latest scope object is null.", x.token);
          return nullptr;
        }

        // Sanity check on key
        if ((key->type() != Constants::TYPE_KEY) || (!key->IsType<Token>()))
        {
          error_tracker_.RaiseRuntimeError("Expected an key identifier.", key->token());
          return nullptr;
        }

        // Creating an entry
        std::string name = static_cast<std::string>(key->As<Token>());
        if (value->type() == Constants::LITERAL)
        {
          object->Insert(name, value->NewInstance());
        }
        else if (value->type() == Constants::TYPE_INSTANCE)
        {
          object->Insert(name, value->As<Vocabulary>());
        }
        else
        {
          error_tracker_.RaiseRuntimeError("Expected instance or literal.", value->token());
          return nullptr;
        }
      }

      break;

      case Constants::SEPARATOR:
        // TODO(private issue AEA-135): Validate
        break;

      case Constants::IDENTIFIER:
      {
        auto name = static_cast<std::string>(x.token);

        QueryVariant ele;
        if (context_.Has(name))
        {
          ele = context_.Get(name);
        }
        else if (semantic_search_module_->HasModel(name))
        {
          auto model = semantic_search_module_->GetModel(name);
          ele        = NewQueryVariant(model, Constants::TYPE_MODEL, x.token);
        }
        else
        {
          ele = NewQueryVariant(name, Constants::TYPE_KEY, x.token);
        }

        stack.push_back(ele);
        break;
      }

      case Constants::OBJECT_KEY:
      {
        QueryVariant ele = NewQueryVariant(x.token, Constants::TYPE_KEY, x.token);
        stack.push_back(ele);
        break;
      }
      case Constants::LITERAL:
      {
        auto const &user_types = semantic_search_module_->type_information();
        bool        found      = false;
        for (auto const &type : user_types)
        {
          if (type.code == x.token.type())
          {
            if (!type.allocator)
            {
              error_tracker_.RaiseInternalError("Literal is missing allocator.", x.token);
              return nullptr;
            }

            // Adding the literal to the stack
            auto element = type.allocator(x.token);
            stack.push_back(element);

            found = true;
            break;
          }
        }

        if (!found)
        {
          error_tracker_.RaiseInternalError("Unable to find literal in custom defined types.",
                                            x.token);
          return nullptr;
        }
        break;
      }
      case Constants::VAR_TYPE:  // Colon use to define model.
      {
        auto model_obj = stack.back();
        stack.pop_back();

        // We leave the instance on the stack so it can be stored
        auto &instance_obj = stack.back();

        if (model_obj->type() != Constants::TYPE_MODEL)
        {
          error_tracker_.RaiseRuntimeError("Expected model.", model_obj->token());
          return nullptr;
        }

        if (!model_obj->IsType<VocabularySchemaPtr>())
        {
          error_tracker_.RaiseRuntimeError("Expected model to be schema.", model_obj->token());
          return nullptr;
        }

        if (!instance_obj->IsType<Vocabulary>())
        {
          error_tracker_.RaiseRuntimeError("Expected right hand side to be an instance.",
                                           instance_obj->token());
          return nullptr;
        }

        auto instance = instance_obj->As<Vocabulary>();
        auto model    = model_obj->As<VocabularySchemaPtr>();

        std::string error;
        if (!model->Validate(instance, error))
        {
          // Reporting error
          error_tracker_.RaiseRuntimeError("Instance does not match model requirements.", x.token);
          error_tracker_.Append(error, x.token);
          return nullptr;
        }

        assert(model->model_name() != "");
        instance->SetModelName(model->model_name());
        break;
      }
      case Constants::ASSIGN:
      {
        auto instance = stack.back();
        stack.pop_back();
        auto model = stack.back();
        stack.pop_back();
        auto name = stack.back();
        stack.pop_back();

        // Reversing the order to prepare for storage
        stack.push_back(name);
        stack.push_back(instance);
        stack.push_back(model);

        break;
      }
      case Constants::STORE_POSITION:
      {
        auto instance_obj = stack.back();
        stack.pop_back();
        auto name = stack.back();
        stack.pop_back();

        auto instance = instance_obj->As<Vocabulary>();
        auto model    = semantic_search_module_->GetModel(instance->model_name());
        if (model == nullptr)
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + instance->model_name() + "'",
                                          x.token);
          return nullptr;
        }
        auto position = model->Reduce(instance);
        auto executor_position =
            NewQueryVariant(position, Constants::TYPE_INSTANCE, instance_obj->token());
        context_.Set(name->As<std::string>(), executor_position, instance->model_name());
        break;
      }
      case Constants::STORE:
      {
        auto instance_obj = stack.back();
        stack.pop_back();
        auto name = stack.back();
        stack.pop_back();

        auto instance = instance_obj->As<Vocabulary>();

        context_.Set(name->As<std::string>(), instance_obj, instance->model_name());
        break;
      }
      case Constants::ADVERTISE:
      {
        auto variant = stack.back();
        stack.pop_back();

        if (!variant->IsType<Vocabulary>())
        {
          auto tok = x.token;
          if (i > 0)
          {
            tok = stmt[i - 1].token;
          }

          error_tracker_.RaiseSyntaxError("Expected Vocabulary.", tok);
          return nullptr;
        }

        auto instance   = variant->As<Vocabulary>();
        auto model_name = instance->model_name();

        if (!semantic_search_module_->HasModel(model_name))
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + model_name + "'",
                                          variant->token());
          return nullptr;
        }

        auto model = semantic_search_module_->GetModel(model_name);
        if (model == nullptr)
        {
          error_tracker_.RaiseInternalError("Model '" + model_name + "' is null.",
                                            variant->token());
          return nullptr;
        }

        model->VisitFields(
            [this, variant](std::string, std::string mname, Vocabulary obj) {
              auto model = semantic_search_module_->GetModel(mname);
              if (model == nullptr)
              {
                error_tracker_.RaiseSyntaxError("Could not find model '" + mname + "'",
                                                variant->token());
                return;
              }
              SemanticPosition position = model->Reduce(obj);

              assert(semantic_search_module_ != nullptr);
              assert(semantic_search_module_->advertisement_register() != nullptr);
              assert(agent_ != nullptr);

              semantic_search_module_->advertisement_register()->AdvertiseAgent(agent_->id, mname,
                                                                                position);
              agent_->RegisterVocabularyLocation(mname, position);
            },
            instance);

        break;
      }
      case Constants::SEARCH:
      {
        if (stack.empty())
        {
          error_tracker_.RaiseInternalError("Not enough elements on stack to search.", x.token);
          return nullptr;
        }

        auto variant = stack.back();
        stack.pop_back();

        if (!variant->IsType<Vocabulary>())
        {
          auto tok = x.token;
          if (i > 0)
          {
            tok = stmt[i - 1].token;
          }

          error_tracker_.RaiseSyntaxError("Expected Vocabulary.", tok);
          return nullptr;
        }

        auto instance = variant->As<Vocabulary>();

        if (!semantic_search_module_->HasModel(instance->model_name()))
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + instance->model_name() + "'",
                                          x.token);
          return nullptr;
        }

        auto model = semantic_search_module_->GetModel(instance->model_name());
        if (model == nullptr)
        {
          error_tracker_.RaiseInternalError("Model '" + instance->model_name() + "' is null.",
                                            x.token);
          return nullptr;
        }

        // TODO: Add support for granularity
        // TODO: Add support for line segment
        auto position = model->Reduce(instance);
        result        = semantic_search_module_->advertisement_register()->FindAgents(
            instance->model_name(), position, 2);
        std::cout << "Searching ... " << instance->model_name() << std::endl;
        if (result)
        {
          std::cout << "Found agents: " << result->size() << " "
                    << " searching " << instance->model_name() << std::endl;
        }
        break;
      }
      case Constants::MULTIPLY:
        break;
      case Constants::ADD:
        break;
      case Constants::SUB:
      {
        std::cout << "Subtracting" << std::endl;
        if (stack.size() < 2)
        {
          error_tracker_.RaiseInternalError("Not enough elements on stack to subtract.", x.token);
          return nullptr;
        }
        auto lhs = stack.back();
        stack.pop_back();

        if (!lhs->IsType<SemanticPosition>())
        {
          error_tracker_.RaiseRuntimeError("Left-hand side must be semantic position.", x.token);
          return nullptr;
        }

        auto rhs = stack.back();
        stack.pop_back();

        if (!rhs->IsType<SemanticPosition>())
        {
          error_tracker_.RaiseRuntimeError("Right-hand side must be semantic position.", x.token);
          return nullptr;
        }

        auto a   = lhs->As<SemanticPosition>();
        auto b   = rhs->As<SemanticPosition>();
        auto c   = a - b;
        auto ret = NewQueryVariant(c, Constants::TYPE_INSTANCE, x.token);
        stack.push_back(ret);
        break;
      }
      default:
        error_tracker_.RaiseInternalError("Unhandled operator.", x.token);
        break;
      }

      ++i;
    }
  }
  catch (std::exception const &e)
  {
    error_tracker_.RaiseRuntimeError(e.what(), stmt[i].token);
    return nullptr;
  }

  return result;
}

QueryExecutor::Vocabulary QueryExecutor::GetInstance(std::string const &name)
{
  assert(context_.Has(name));
  auto var = context_.Get(name);
  return var->As<Vocabulary>();
}

QueryExecutor::AgentIdSet QueryExecutor::ExecuteStore(CompiledStatement const &stmt)
{
  if (stmt[1].type != Constants::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable name.", stmt[1].token);
    return nullptr;
  }

  auto name = static_cast<std::string>(stmt[1].token);

  if (!context_.Has(name))
  {
    error_tracker_.RaiseSyntaxError("Vairable not defined", stmt[1].token);
    return nullptr;
  }

  auto obj        = context_.Get(name);
  auto model_name = context_.GetModelName(name);

  if (!semantic_search_module_->HasModel(model_name))
  {
    error_tracker_.RaiseSyntaxError("Could not find model '" + model_name + "'", stmt[1].token);
    return nullptr;
  }

  auto model = semantic_search_module_->GetModel(model_name);
  if (model == nullptr)
  {
    error_tracker_.RaiseInternalError("Model '" + model_name + "' is null.", stmt[1].token);
    return nullptr;
  }

  // TODO: Check instance type
  auto instance = obj->As<Vocabulary>();

  model->VisitFields(
      [this, stmt](std::string, std::string mname, Vocabulary obj) {
        auto model = semantic_search_module_->GetModel(mname);
        if (model == nullptr)
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + mname + "'", stmt[1].token);
          return;
        }
        SemanticPosition position = model->Reduce(obj);

        assert(semantic_search_module_ != nullptr);
        assert(semantic_search_module_->advertisement_register() != nullptr);
        assert(agent_ != nullptr);

        semantic_search_module_->advertisement_register()->AdvertiseAgent(agent_->id, mname,
                                                                          position);
        agent_->RegisterVocabularyLocation(mname, position);
      },
      instance);

  return nullptr;
}

QueryExecutor::AgentIdSet QueryExecutor::ExecuteSet(CompiledStatement const &stmt)
{
  std::size_t i = 3;

  if (stmt.size() < i)
  {
    error_tracker_.RaiseSyntaxError("Set statment has incorrect syntax.", stmt[0].token);
    return nullptr;
  }

  if (stmt[1].type != Constants::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable name.", stmt[1].token);
    return nullptr;
  }

  if (stmt[2].type != Constants::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable type.", stmt[2].token);
    return nullptr;
  }

  std::vector<QueryVariant> stack;
  std::vector<Vocabulary>   scope_objects;
  Vocabulary                last;
  int                       scope_depth = 0;

  QueryVariant object_name =
      NewQueryVariant(static_cast<std::string>(stmt[1].token), Constants::TYPE_KEY, stmt[1].token);
  stack.push_back(object_name);

  QueryVariant model_name =
      NewQueryVariant(static_cast<std::string>(stmt[2].token), Constants::TYPE_KEY, stmt[2].token);
  stack.push_back(model_name);

  while (i < stmt.size())
  {
    auto x = stmt[i];
    switch (x.type)
    {
    case Constants::PUSH_SCOPE:
    {
      ++scope_depth;
      auto obj = VocabularyInstance::New<PropertyMap>({});
      scope_objects.push_back(obj);
      break;
    }
    case Constants::POP_SCOPE:
      --scope_depth;

      if (scope_objects.empty())
      {
      }
      last = scope_objects.back();
      scope_objects.pop_back();

      {
        QueryVariant s =
            NewQueryVariant(last, Constants::TYPE_INSTANCE,
                            x.token);  // TODO(private issue AEA-132): Nested shared_ptr
        stack.push_back(s);
      }

      break;

    case Constants::ATTRIBUTE:
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
      if (value->type() == Constants::LITERAL)
      {
        object->Insert(name, value->NewInstance());
      }
      else if (value->type() == Constants::TYPE_INSTANCE)
      {
        object->Insert(name, value->As<Vocabulary>());
      }
      else
      {
        std::cerr << "TODO: Yet another error: " << value->type() << " " << value->token()
                  << std::endl;
      }
    }

    break;

    case Constants::SEPARATOR:
      // TODO(private issue AEA-135): Validate
      break;

    case Constants::IDENTIFIER:
    {
      auto ele = context_.Get(static_cast<std::string>(x.token));

      stack.push_back(ele);
      break;
    }

    case Constants::OBJECT_KEY:
    {
      QueryVariant ele = NewQueryVariant(x.token, Constants::TYPE_KEY, x.token);
      stack.push_back(ele);
      break;
    }
    case Constants::LITERAL:
    {
      auto const &user_types = semantic_search_module_->type_information();
      bool        found      = false;
      for (auto const &type : user_types)
      {
        if (type.code == x.token.type())
        {
          if (!type.allocator)
          {
            std::cerr << "TODO: able to parse but not allocate " << x.token;
            return nullptr;
          }

          auto element = type.allocator(x.token);
          stack.push_back(element);

          found = true;
          break;
        }
      }
      if (!found)
      {
        std::cout << "'" << x.token << "'  - TODO 2 IMPLEMENT " << x.type
                  << std::endl;  // TODO(private issue AEA-140): Handle this case
      }
      break;
    }
    case Constants::VAR_TYPE:
      // TODO: Check correctness
      std::cout << "TODO - deal with this case" << std::endl;
      break;
    default:
      // TODO: Deal with equality
      std::cout << "'" << x.token << "'  - TODO IMPLEMENT " << x.type
                << std::endl;  // TODO(private issue AEA-140): Handle this case
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
      return nullptr;
    }

    std::string error;
    if (!model->Validate(last, error))
    {
      // Reporting error
      error_tracker_.RaiseRuntimeError("Instance does not match model requirements.",
                                       stmt[stmt.size() - 1].token);

      error_tracker_.Append(error, stmt[stmt.size() - 1].token);
      return nullptr;
    }

    //    context_.Set(key->As<std::string>(), last, name_of_model);
  }
  else
  {
    error_tracker_.RaiseRuntimeError(
        "No object instance was created, only declared. This is not supported yet.",
        stmt[stmt.size() - 1].token);
    return nullptr;
  }

  return nullptr;
}

QueryExecutor::AgentIdSet QueryExecutor::ExecuteDefine(CompiledStatement const &stmt)
{
  std::size_t i = 1;

  definition_stack_.clear();
  std::vector<ModelInterfaceBuilder> scope_models;
  ModelInterfaceBuilder              last;
  int                                scope_depth = 0;

  while (i < stmt.size())
  {
    auto x = stmt[i];

    switch (x.type)
    {
    case Constants::PUSH_SCOPE:
      ++scope_depth;
      scope_models.push_back(semantic_search_module_->NewProxy());
      break;

    case Constants::POP_SCOPE:

      --scope_depth;
      // TODO(private issue AEA-131): Check enough
      last = scope_models.back();
      scope_models.pop_back();

      {
        QueryVariant s = NewQueryVariant(last.vocabulary_schema(), Constants::TYPE_MODEL, x.token);
        definition_stack_.push_back(s);
      }

      break;

    case Constants::ATTRIBUTE:
    {
      QueryVariant value = definition_stack_.back();
      definition_stack_.pop_back();
      QueryVariant key = definition_stack_.back();
      definition_stack_.pop_back();

      auto model = scope_models.back();

      // Asserting types
      if (TypeMismatch<ModelField>(value, x.token))
      {
        error_tracker_.RaiseInternalError("Model field value type mismatch", x.token);
        return nullptr;
      }

      if (TypeMismatch<Token>(key, x.token))
      {
        error_tracker_.RaiseInternalError("Model field key type mismatch", x.token);
        return nullptr;
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

    case Constants::SEPARATOR:
      // TODO(private issue AEA-135): Validate
      break;

    case Constants::IDENTIFIER:
      if (scope_depth == 0)
      {
        QueryVariant ele = NewQueryVariant(x.token, Constants::TYPE_KEY, x.token);
        definition_stack_.push_back(ele);
      }
      else
      {
        auto field_name = static_cast<std::string>(x.token);
        if (!semantic_search_module_->HasField(field_name))
        {
          error_tracker_.RaiseRuntimeError(
              "Could not find field: " + static_cast<std::string>(x.token), x.token);
          return nullptr;
        }

        QueryVariant ele = NewQueryVariant(semantic_search_module_->GetField(field_name),
                                           Constants::TYPE_MODEL, x.token);
        definition_stack_.push_back(ele);
      }

      break;
    case Constants::OBJECT_KEY:
    {
      QueryVariant ele = NewQueryVariant(x.token, Constants::TYPE_KEY, x.token);
      definition_stack_.push_back(ele);
      break;
    }
    case Constants::FUNCTION:
    {
      QueryVariant ele = NewQueryVariant(x.token, Constants::TYPE_FUNCTION_NAME, x.token);
      definition_stack_.push_back(ele);
      break;
    }
    case Constants::EXECUTE_CALL:
    {
      if (definition_stack_.empty())
      {
        error_tracker_.RaiseInternalError("Stack is empty", x.token);
        return nullptr;
      }

      std::vector<void const *>    args;
      std::vector<std::type_index> arg_signature;

      // Function arguments
      uint64_t n = definition_stack_.size() - 1;
      while ((n != 0) && (definition_stack_[n]->type() != Constants::TYPE_FUNCTION_NAME))
      {
        auto arg = definition_stack_[n];
        args.push_back(arg->data());
        arg_signature.push_back(arg->type_index());

        --n;
      }

      // Ordering the function arguments correctly
      std::reverse(args.begin(), args.end());
      std::reverse(arg_signature.begin(), arg_signature.end());

      // Calling
      if (TypeMismatch<Token>(definition_stack_[n], definition_stack_[n]->token()))
      {
        return nullptr;
      }
      auto function_name = static_cast<std::string>(definition_stack_[n]->As<Token>());

      if (!semantic_search_module_->HasFunction(function_name))
      {
        error_tracker_.RaiseRuntimeError("Function '" + function_name + "' does not exist.",
                                         definition_stack_[n]->token());
        return nullptr;
      }

      auto &          function = (*semantic_search_module_)[function_name];
      std::type_index ret_type = function.return_type();

      if (!function.ValidateSignature(ret_type, arg_signature))
      {
        error_tracker_.RaiseRuntimeError(
            "Call to '" + function_name + "' does not match signature.",
            definition_stack_[n]->token());
        return nullptr;
      }

      QueryVariant ret;
      try
      {
        ret = function(args);
      }
      catch (std::exception const &e)
      {
        error_tracker_.RaiseRuntimeError(e.what(), definition_stack_[n]->token());
        return nullptr;
      }

      // Clearing stack
      for (std::size_t j = 0; j < args.size(); ++j)
      {
        definition_stack_.pop_back();
      }
      definition_stack_.pop_back();  // Function name

      // Pushing result
      definition_stack_.push_back(ret);
      break;
    }
    default:
    {  // Scanning user types
      auto const &user_types = semantic_search_module_->type_information();
      bool        found      = false;
      for (auto const &type : user_types)
      {
        if (type.code == x.token.type())
        {
          if (!type.allocator)
          {
            std::cerr << "TODO: able to parse but not allocate " << x.token;
            return nullptr;
          }

          auto element = type.allocator(x.token);
          definition_stack_.push_back(element);

          found = true;
          break;
        }
      }

      if (!found)
      {
        // TODO: Implement missing
        std::cout << "'" << x.token << "'  - TODO IMPLEMENT 33"
                  << std::endl;  // TODO(private issue AEA-140): Handle this case
      }
      break;
    }
    }

    ++i;
  }

  if (bool(last))
  {
    auto value = definition_stack_.back();
    definition_stack_.pop_back();
    auto key = definition_stack_.back();
    definition_stack_.pop_back();

    // TODO(private issue AEA-137): add some sanity checks here
    auto token = key->As<Token>();
    auto name  = static_cast<std::string>(token);
    try
    {
      semantic_search_module_->AddModel(name, last.vocabulary_schema());
    }
    catch (std::runtime_error const &e)
    {
      error_tracker_.RaiseRuntimeError(e.what(), token);
    }
  }
  else
  {
    // TODO(private issue AEA-140): Handle this case
    std::cout << "NOT READY: " << last.vocabulary_schema() << std::endl;
  }

  return nullptr;
}

QueryExecutor::AgentIdSet QueryExecutor::ExecuteFind(CompiledStatement const &stmt)
{
  if (stmt[1].type != Constants::IDENTIFIER)
  {
    error_tracker_.RaiseSyntaxError("Expected variable name.", stmt[1].token);
    return nullptr;
  }

  auto name = static_cast<std::string>(stmt[1].token);

  if (!context_.Has(name))
  {
    error_tracker_.RaiseSyntaxError("Variable not defined", stmt[1].token);
    return nullptr;
  }

  auto obj        = context_.Get(name);
  auto model_name = context_.GetModelName(name);

  if (!semantic_search_module_->HasModel(model_name))
  {
    error_tracker_.RaiseSyntaxError("Could not find model '" + model_name + "'", stmt[1].token);
    return nullptr;
  }

  auto model = semantic_search_module_->GetModel(model_name);
  if (model == nullptr)
  {
    error_tracker_.RaiseInternalError("Model '" + model_name + "' is null.", stmt[1].token);
    return nullptr;
  }

  // TODO: Check
  auto instance = obj->As<Vocabulary>();

  assert(semantic_search_module_->advertisement_register() != nullptr);
  assert(agent_ != nullptr);
  auto position = model->Reduce(instance);
  auto results =
      semantic_search_module_->advertisement_register()->FindAgents(model_name, position, 2);

  return results;
}

}  // namespace semanticsearch
}  // namespace fetch
