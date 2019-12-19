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
#include "semanticsearch/schema/model_identifier.hpp"
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
  mode_  = Mode::INSTANTIATION;

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

  // Clearing state
  context_.Clear();
  error_tracker_.ClearErrors();

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
  locals_numbers_.clear();
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

        QueryVariant    ele;
        ModelIdentifier model_identifier;
        model_identifier.scope      = scope_;
        model_identifier.model_name = name;
        if (context_.Has(name))
        {
          ele = context_.Get(name);
        }
        else if (semantic_search_module_->HasModel(model_identifier))
        {
          auto model = semantic_search_module_->GetModel(model_identifier);
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
      case Constants::VAR_TYPE:
      {
        auto model_obj = stack.back();
        stack.pop_back();

        // We leave the instance on the stack so it can be stored
        auto &instance_obj = stack.back();

        if (model_obj->type() != Constants::TYPE_MODEL)
        {
          std::stringstream ss{""};
          ss << "Expected model, but " << model_obj->token() << " is not a model in "
             << scope_.address << "@" << scope_.version << ".";
          error_tracker_.RaiseRuntimeError(ss.str(), model_obj->token());
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
        // TODO: Make support for optional model - i.e. fetch it if it is not there
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

        auto            instance = instance_obj->As<Vocabulary>();
        ModelIdentifier model_identifier;
        model_identifier.scope      = scope_;
        model_identifier.model_name = instance->model_name();

        auto model = semantic_search_module_->GetModel(model_identifier);
        if (model == nullptr)
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + instance->model_name() + "'",
                                          x.token);
          return nullptr;
        }

        // Creating position
        auto position = model->Reduce(instance);
        position.SetModelName(instance->model_name());

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
      case Constants::VERSION:
        // Fall-through is deliberate.
      case Constants::GRANULARITY:
        // Fall-through is deliberate.
      case Constants::UNTIL:
      {
        auto value_obj = stack.back();
        stack.pop_back();
        // auto &variant = stack.back();

        if (value_obj->type() != Constants::LITERAL)
        {
          error_tracker_.RaiseRuntimeError("Expected literal.", value_obj->token());
          return nullptr;
        }

        // Converting to literal
        auto type_code    = value_obj->token().type();  // TODO: Move to function -- replicated 1
        auto type_details = semantic_search_module_->GetTypeDetails(type_code);

        if (!type_details.semantic_converter)
        {
          error_tracker_.RaiseRuntimeError("No known conversion from literal into semantic space.",
                                           value_obj->token());
          return nullptr;
        }

        auto value = type_details.semantic_converter(value_obj);

        switch (x.type)
        {
        case Constants::GRANULARITY:
          locals_numbers_[LOCAL_GRANULARITY] = value;
          break;
        case Constants::UNTIL:
          locals_numbers_[LOCAL_BLOCK_EXPIRY] = value;
          break;
        case Constants::VERSION:
          locals_numbers_[LOCAL_VERSION] = value;
          break;
        };

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

          error_tracker_.RaiseRuntimeError("Expected Vocabulary.", tok);
          return nullptr;
        }

        auto            instance   = variant->As<Vocabulary>();
        auto            model_name = instance->model_name();
        ModelIdentifier model_identifier;
        model_identifier.scope      = scope_;
        model_identifier.model_name = model_name;

        if (!semantic_search_module_->HasModel(model_identifier))
        {
          error_tracker_.RaiseSyntaxError("Could not find model '" + model_name + "'",
                                          variant->token());
          return nullptr;
        }

        auto model = semantic_search_module_->GetModel(model_identifier);
        if (model == nullptr)
        {
          error_tracker_.RaiseInternalError("Model '" + model_name + "' is null.",
                                            variant->token());
          return nullptr;
        }

        auto life_time =
            GetLocal(LOCAL_BLOCK_EXPIRY, SemanticCoordinateType(current_block_time_ + 20))
                .Integer();

        std::cout << "Advertising " << model_identifier << " until " << life_time << std::endl;
        // TODO: Use lifetime and model identifier

        model->VisitFields(
            [this, variant](std::string, std::string mname, Vocabulary obj) {
              ModelIdentifier mi;
              mi.scope      = scope_;
              mi.model_name = mname;

              auto model = semantic_search_module_->GetModel(mi);
              if (model == nullptr)
              {
                error_tracker_.RaiseSyntaxError(
                    "Could not find model '" + static_cast<std::string>(mi) + "'",
                    variant->token());
                return;
              }
              SemanticPosition position = model->Reduce(obj);

              assert(semantic_search_module_ != nullptr);
              assert(semantic_search_module_->advertisement_register() != nullptr);
              assert(agent_ != nullptr);

              semantic_search_module_->advertisement_register()->AdvertiseAgent(agent_->id, mi,
                                                                                position);
              agent_->RegisterVocabularyLocation(mname,
                                                 position);  // TODO: Update with model identifier
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

        SemanticPosition position;
        bool             found{false};
        ModelIdentifier  model_identifier;
        model_identifier.scope = scope_;

        // Checking if it is a model instance
        if (variant->IsType<Vocabulary>())
        {
          auto instance               = variant->As<Vocabulary>();
          model_identifier.model_name = instance->model_name();

          if (!semantic_search_module_->HasModel(model_identifier))
          {
            error_tracker_.RaiseSyntaxError("Could not find model '" + instance->model_name() + "'",
                                            x.token);
            return nullptr;
          }

          auto model = semantic_search_module_->GetModel(model_identifier);
          if (model == nullptr)
          {
            error_tracker_.RaiseInternalError("Model '" + instance->model_name() + "' is null.",
                                              x.token);
            return nullptr;
          }

          // Populating parameters
          model_identifier.model_name = instance->model_name();
          position                    = model->Reduce(instance);
          found                       = true;
        }

        // Checking if it is a model instance
        if (variant->IsType<SemanticPosition>())
        {
          position                    = variant->As<SemanticPosition>();
          model_identifier.model_name = position.model_name();
          found                       = true;
        }

        // Raising error if stack parameters are wrong
        if (!found)
        {
          auto tok = x.token;
          if (i > 0)
          {
            tok = stmt[i - 1].token;
          }

          error_tracker_.RaiseSyntaxError("Expected model instance or position.", tok);
          return nullptr;
        }

        auto granularity = GetLocal(LOCAL_GRANULARITY, default_granularity_).Integer();
        //        auto limit       = GetLocal(LOCAL_LIMIT, default_granularity_).Integer();
        //        auto max_depth   = GetLocal(LOCAL_MAX_DEPTH, default_max_depth_).Integer();

        // Searching
        // TODO: Finish this section

        std::cout << "Searching ... " << model_identifier << std::endl;

        result = semantic_search_module_->advertisement_register()->FindAgents(
            model_identifier, position, static_cast<int32_t>(granularity));

        if (result)
        {
          std::cout << "Found agents: " << result->size() << " "
                    << " searching " << model_identifier.model_name << std::endl;
        }
        else
        {
          std::cout << "Nothing found" << std::endl;
        }
        break;
      }
      case Constants::USING:
      {
        if (stack.size() < 1)
        {
          error_tracker_.RaiseInternalError("Expected an identifier after using statement.",
                                            x.token);
          return nullptr;
        }
        auto identifier = stack.back();
        stack.pop_back();

        scope_.address = identifier->As<std::string>();
        scope_.version = GetLocal(LOCAL_VERSION, SemanticCoordinateType(-1));
        mode_          = Mode::INSTANTIATION;

        break;
      }
      case Constants::SPECIFICATION:
      {
        if (stack.size() < 1)
        {
          error_tracker_.RaiseInternalError("Expected an identifier after specification statement.",
                                            x.token);
          return nullptr;
        }
        auto identifier = stack.back();
        stack.pop_back();

        scope_.address = identifier->As<std::string>();
        scope_.version = GetLocal(LOCAL_VERSION, SemanticCoordinateType(-1));
        mode_          = Mode::SPECIFICATION;

        break;
      }
      case Constants::MULTIPLY:
      {
        if (stack.size() < 2)
        {
          error_tracker_.RaiseInternalError("Not enough elements on stack to subtract.", x.token);
          return nullptr;
        }
        int  rhs_type = 0;
        auto rhs      = stack.back();
        stack.pop_back();

        if (rhs->IsType<SemanticPosition>())
        {
          rhs_type = 1;
        }

        if (rhs->IsType<SemanticCoordinateType>())
        {
          rhs_type          = 2;
          auto type_code    = rhs->token().type();  // TODO: Move to function -- replicated 1
          auto type_details = semantic_search_module_->GetTypeDetails(type_code);

          if (!type_details.semantic_converter)
          {
            error_tracker_.RaiseRuntimeError(
                "No known conversion from literal into semantic space.", rhs->token());
            return nullptr;
          }
        }

        if (rhs_type == 0)
        {
          error_tracker_.RaiseRuntimeError("Right-hand side must be semantic position or literal.",
                                           x.token);
          return nullptr;
        }

        int  lhs_type = 0;
        auto lhs      = stack.back();
        stack.pop_back();

        if (lhs->IsType<SemanticPosition>())
        {
          lhs_type = 4;
        }

        if (lhs->IsType<SemanticCoordinateType>())
        {
          lhs_type = 8;
        }

        if (lhs_type == 0)
        {
          error_tracker_.RaiseRuntimeError("Right-hand side must be semantic position or literal.",
                                           x.token);
          return nullptr;
        }

        switch (lhs_type + rhs_type)
        {
        case 5:
        {
          error_tracker_.RaiseRuntimeError("Cannot multiply vector with vector.", x.token);
          return nullptr;
        }
        case 6:  // vector - scalar
        {
          assert(rhs->IsType<SemanticCoordinateType>());
          auto scalar = rhs->As<SemanticCoordinateType>();
          auto vec    = lhs->As<SemanticPosition>();
          auto c      = scalar * vec;
          c.SetModelName(vec.model_name());
          stack.push_back(NewQueryVariant(c, Constants::TYPE_INSTANCE, x.token));
          break;
        }
        case 9:  // scalar - vector
        {
          assert(lhs->IsType<SemanticCoordinateType>());
          auto scalar = lhs->As<SemanticCoordinateType>();
          auto vec    = rhs->As<SemanticPosition>();
          auto c      = scalar * vec;
          c.SetModelName(vec.model_name());
          stack.push_back(NewQueryVariant(c, Constants::TYPE_INSTANCE, x.token));
          break;
        }
        case 10:  // scalar - scalar
        {
          assert(rhs->IsType<SemanticCoordinateType>());
          assert(lhs->IsType<SemanticCoordinateType>());
          auto s1 = rhs->As<SemanticCoordinateType>();
          auto s2 = lhs->As<SemanticCoordinateType>();
          auto c  = s1 * s2;
          stack.push_back(NewQueryVariant(c, Constants::TYPE_INSTANCE, x.token));
          break;
        }
        }

        break;
      }

      case Constants::ADD:
      case Constants::SUB:
      {
        // Sanity checking
        if (stack.size() < 2)
        {
          error_tracker_.RaiseInternalError("Not enough elements on stack to subtract.", x.token);
          return nullptr;
        }
        auto rhs = stack.back();
        stack.pop_back();

        if (!rhs->IsType<SemanticPosition>())
        {
          error_tracker_.RaiseRuntimeError("Right-hand side must be semantic position.", x.token);
          return nullptr;
        }

        auto lhs = stack.back();
        stack.pop_back();

        if (!lhs->IsType<SemanticPosition>())
        {
          error_tracker_.RaiseRuntimeError("Left-hand side must be semantic position.", x.token);
          return nullptr;
        }

        auto a = lhs->As<SemanticPosition>();
        auto b = rhs->As<SemanticPosition>();

        if (a.model_name() != b.model_name())
        {
          error_tracker_.RaiseRuntimeError(
              "Cannot manipulate two vectors derived from different models even if rank is the "
              "same.",
              x.token);
          return nullptr;
        }

        // Logic
        SemanticPosition c;

        switch (x.type)
        {
        case Constants::SUB:
          c = a - b;
          break;
        case Constants::ADD:
          c = a + b;
          break;
        }

        c.SetModelName(a.model_name());

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
}  // namespace semanticsearch

QueryExecutor::Vocabulary QueryExecutor::GetInstance(std::string const &name)
{
  assert(context_.Has(name));
  auto var = context_.Get(name);
  return var->As<Vocabulary>();
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

        ModelIdentifier model_identifier;
        model_identifier.scope      = scope_;
        model_identifier.model_name = static_cast<std::string>(x.token);

        if (!semantic_search_module_->HasField(model_identifier))
        {
          error_tracker_.RaiseRuntimeError(
              "Could not find field: " + static_cast<std::string>(x.token), x.token);
          return nullptr;
        }

        QueryVariant ele = NewQueryVariant(semantic_search_module_->GetField(model_identifier),
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
    auto            token = key->As<Token>();
    auto            name  = static_cast<std::string>(token);
    ModelIdentifier model_identifier;
    model_identifier.scope      = scope_;
    model_identifier.model_name = name;
    try
    {
      semantic_search_module_->AddModel(model_identifier, last.vocabulary_schema());
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

}  // namespace semanticsearch
}  // namespace fetch
