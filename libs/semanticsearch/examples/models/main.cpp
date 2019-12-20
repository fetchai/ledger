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

#include <core/random/lfg.hpp>
#include <exception>
#include <iostream>

#include <chrono>
#include <fstream>
#include <memory>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace fetch::semanticsearch;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cout << "specify file" << std::endl;
    exit(-1);
  }

  auto adv = std::make_shared<ModelAdvertisementRegister>();

  auto semantic_search_module = SemanticSearchModule::New(adv);

  std::string   filename = argv[1];
  std::ifstream t(filename);
  if (!t)
  {
    std::cerr << "Could not load file" << std::endl;
    exit(-1);
  }

  std::string source((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

  // Creating a query
  ErrorTracker  error_tracker;
  QueryCompiler compiler(error_tracker, semantic_search_module);
  auto          query = compiler(source, filename);
  if (error_tracker)
  {
    std::cout << "Errors during compilation" << std::endl;
    error_tracker.Print();
    return -1;
  }

  // Executing the query
  semantic_search_module->RegisterAgent("agent1");
  semantic_search_module->RegisterAgent("agent2");
  semantic_search_module->RegisterAgent("agent3");

  QueryExecutor exe(
      semantic_search_module,
      error_tracker);  // TODO(private issue AEA-128): Need to pass collection, not single instance

  // Executing query on behalf of agent2
  auto agent = semantic_search_module->GetAgent("agent2");
  exe.Execute(query, agent);

  if (error_tracker)
  {
    std::cout << "Errors during execution" << std::endl;
    error_tracker.Print();
    return -1;
  }

  return 0;
}
