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

#include "http/json_response.hpp"
#include "http/module.hpp"

#include <atomic>

namespace fetch {
namespace constellation {

class HealthCheckHttpModule : public http::HTTPModule
{
public:
  using BlockCoordinator    = ledger::BlockCoordinator;
  using MainChain           = ledger::MainChain;
  using MainChainRpcService = ledger::MainChainRpcService;

  HealthCheckHttpModule(MainChain const &chain, BlockCoordinator const &block_coordinator)
    : chain_{chain}
    , block_coordinator_{block_coordinator}
  {
    Get("/api/health/alive", "Endpoint to check if the server is alive.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });

    Get("/api/health/ready", "Retrieves the current synchronisation status.",
        [this](http::ViewParameters const &, http::HTTPRequest const &) {
          MainChainRpcService const *const chain_service = chain_service_.load();

          // determine the state of the machine system state machines
          bool const chain_synced = (chain_service != nullptr) && chain_service->IsSynced();
          bool const chain_executed_finished =
              block_coordinator_.GetStateMachine().state() == BlockCoordinator::State::SYNCHRONISED;
          bool const chain_execution_complete =
              block_coordinator_.GetLastExecutedBlock() == chain_.GetHeaviestBlockHash();

          variant::Variant response            = variant::Variant::Object();
          response["chain_synced"]             = chain_synced;
          response["chain_executed_finished"]  = chain_executed_finished;
          response["chain_execution_complete"] = chain_execution_complete;

          // determine the status code for the response
          http::Status status{http::Status::CLIENT_ERROR_PRECONDITION_FAILED};
          if (chain_synced && chain_executed_finished && chain_execution_complete)
          {
            status = http::Status::SUCCESS_OK;
          }

          return http::CreateJsonResponse(response, status);
        });
  }

  void UpdateChainService(MainChainRpcService const &chain_service)
  {
    chain_service_ = &chain_service;
  }

private:
  using MainChainRpcServicePtr = std::atomic<MainChainRpcService const *>;

  MainChain const &       chain_;
  MainChainRpcServicePtr  chain_service_{nullptr};
  BlockCoordinator const &block_coordinator_;
};

}  // namespace constellation
}  // namespace fetch
