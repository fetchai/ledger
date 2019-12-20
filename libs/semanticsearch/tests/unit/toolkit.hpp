#include "semanticsearch/query/query_compiler.hpp"
#include "semanticsearch/query/query_executor.hpp"

struct SemanticSearchToolkit
{
  using SemanticSearchModule          = fetch::semanticsearch::SemanticSearchModule;
  using ModelAdvertisementRegister    = fetch::semanticsearch::ModelAdvertisementRegister;
  using ModelAdvertisementRegisterPtr = std::shared_ptr<ModelAdvertisementRegister>;
  using SemanticSearchModulePtr       = SemanticSearchModule::SemanticSearchModulePtr;
  using Query                         = fetch::semanticsearch::Query;
  using ConstByteArray                = fetch::byte_array::ConstByteArray;
  using ErrorTracker                  = fetch::semanticsearch::ErrorTracker;
  using QueryCompiler                 = fetch::semanticsearch::QueryCompiler;
  using QueryExecutor                 = fetch::semanticsearch::QueryExecutor;

  SemanticSearchToolkit()
    : advertisement{std::make_shared<ModelAdvertisementRegister>()}
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

  ErrorTracker                  error_tracker;
  ModelAdvertisementRegisterPtr advertisement;
  SemanticSearchModulePtr       semantic_search_module;
  QueryCompiler                 compiler;
};
