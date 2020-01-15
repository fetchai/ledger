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

#include "semanticsearch/query/query_compiler.hpp"
#include "semanticsearch/query/query_executor.hpp"

struct SemanticSearchToolkit
{
  using SemanticSearchModule     = fetch::semanticsearch::SemanticSearchModule;
  using AdvertisementRegister    = fetch::semanticsearch::AdvertisementRegister;
  using AdvertisementRegisterPtr = std::shared_ptr<AdvertisementRegister>;
  using SemanticSearchModulePtr  = SemanticSearchModule::SemanticSearchModulePtr;
  using Query                    = fetch::semanticsearch::Query;
  using ConstByteArray           = fetch::byte_array::ConstByteArray;
  using ErrorTracker             = fetch::semanticsearch::ErrorTracker;
  using QueryCompiler            = fetch::semanticsearch::QueryCompiler;
  using QueryExecutor            = fetch::semanticsearch::QueryExecutor;

  SemanticSearchToolkit()
    : advertisement{std::make_shared<AdvertisementRegister>()}
    , semantic_search_module{SemanticSearchModule::New(advertisement)}
    , compiler{error_tracker, semantic_search_module}
  {}

  Query Compile(std::string const &source, std::string const &filename)
  {
    error_tracker.ClearErrors();
    auto query = compiler(source, filename);
    return query;
  }

  bool HasErrors() const
  {
    return error_tracker.HasErrors();
  }

  void PrintErrors()
  {
    if (error_tracker)
    {
      error_tracker.Print();
    }
  }

  void Execute(Query query, ConstByteArray const &agent_pk)
  {
    QueryExecutor exe(semantic_search_module,
                      error_tracker);  // TODO(private issue AEA-128): Need to pass collection, not
                                       // single instance
    auto agent = semantic_search_module->GetAgent(agent_pk);
    // Checking that the agent acutally exists
    if (agent == nullptr)
    {
      // Setting error occurance to very beginning of file
      fetch::byte_array::Token zero;
      zero.SetLine(0);
      zero.SetChar(0);
      error_tracker.RaiseRuntimeError("Agent " + static_cast<std::string>(agent_pk.ToBase64()) +
                                          " not found. Did you remember to register it?",
                                      zero);
      return;
    }

    exe.Execute(query, agent);
  }

  void RegisterAgent(ConstByteArray const &agent_pk)
  {
    semantic_search_module->RegisterAgent(agent_pk);
  }

  void ClearContext()
  {
    // TODO: Implemnent
  }

  ErrorTracker             error_tracker;
  AdvertisementRegisterPtr advertisement;
  SemanticSearchModulePtr  semantic_search_module;
  QueryCompiler            compiler;
};
