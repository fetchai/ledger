#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iostream>
#include <string>

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "ledger/main_chain_node.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_agent_api_impl.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_node.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "python/worker/python_worker.hpp"

#include <string>
#include <time.h>
#include <unistd.h>

namespace fetch {
namespace swarm {

class PySwarm
{
public:
  PySwarm(const PySwarm &rhs)
  {
    nn_core_         = rhs.nn_core_;
    rnd_             = rhs.rnd_;
    swarm_agent_api_ = rhs.swarm_agent_api_;
    swarm_node_      = rhs.swarm_node_;
    worker_          = rhs.worker_;
    chain_node_      = rhs.chain_node_;
  }
  PySwarm(PySwarm &&rhs)
  {
    nn_core_         = std::move(rhs.nn_core_);
    rnd_             = std::move(rhs.rnd_);
    swarm_agent_api_ = std::move(rhs.swarm_agent_api_);
    swarm_node_      = std::move(rhs.swarm_node_);
    worker_          = std::move(rhs.worker_);
    chain_node_      = std::move(rhs.chain_node_);
  }
  PySwarm operator=(const PySwarm &rhs) = delete;
  PySwarm operator=(PySwarm &&rhs)             = delete;
  bool    operator==(const PySwarm &rhs) const = delete;
  bool    operator<(const PySwarm &rhs) const  = delete;

  using mutex_type = std::recursive_mutex;
  using lock_type  = std::lock_guard<std::recursive_mutex>;

  virtual void Start()
  {
    lock_type lock(mutex_);
    swarm_agent_api_->Start();
    nn_core_->Start();
    chain_node_->StartMining();
  }

  virtual void Stop()
  {
    lock_type lock(mutex_);
    nn_core_->Stop();
    swarm_agent_api_->Stop();
  }

  fetch::chain::MainChain::BlockHash blockIdToHash(const std::string &id) const
  {
    return fetch::byte_array::FromHex(id.c_str());
  }

  std::string hashToBlockId(const fetch::chain::MainChain::BlockHash &hash) const
  {
    return std::string(fetch::byte_array::ToHex(hash));
  }

  explicit PySwarm(uint32_t id, uint16_t rpcPort, uint16_t httpPort, uint32_t maxpeers,
                   uint32_t idlespeed, int target, int chainident)
  {
    std::string                     identifier = "node-" + std::to_string(id);
    std::string                     myHost     = "127.0.0.1:" + std::to_string(rpcPort);
    fetch::swarm::SwarmPeerLocation myHostLoc(myHost);

    auto worker  = std::make_shared<PythonWorker>();
    auto nn_core = std::make_shared<fetch::network::NetworkNodeCore>(20, httpPort, rpcPort);
    auto rnd     = std::make_shared<fetch::swarm::SwarmRandom>(id);
    auto swarm_node =
        std::make_shared<fetch::swarm::SwarmNode>(nn_core, identifier, maxpeers, myHost);

    auto http_module = std::make_shared<SwarmHttpModule>(swarm_node);
    nn_core->AddModule(http_module);

    auto chain_node =
        std::make_shared<fetch::ledger::MainChainNode>(nn_core, id, target, chainident);
    auto swarm_agent_api =
        std::make_shared<fetch::swarm::SwarmAgentApiImpl<PythonWorker>>(worker, myHost, idlespeed);
    worker->UseCore(nn_core);

    fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([]() { return time(0); });

    http_module_     = http_module;
    nn_core_         = nn_core;
    rnd_             = rnd;
    swarm_agent_api_ = swarm_agent_api;
    swarm_node_      = swarm_node;
    worker_          = worker;
    chain_node_      = chain_node;

    nn_core_->Start();

    // TODO(kll) Move this setup code somewhere more sensible.
    swarm_agent_api->ToPing([swarm_agent_api, nn_core, swarm_node](
                                fetch::swarm::SwarmAgentApi &unused, const std::string &host) {
      swarm_node->Post([swarm_agent_api, swarm_node, nn_core, host]() {
        try
        {
          auto client = nn_core->ConnectTo(host);
          if (!client)
          {
            swarm_agent_api->DoPingFailed(host);
            return;
          }
          auto newPeer = swarm_node->AskPeerForPeers(host, client);
          if (newPeer.length())
          {
            if (!swarm_node->IsOwnLocation(newPeer))
            {
              if (!swarm_node->IsExistingPeer(newPeer))
              {
                swarm_node->AddOrUpdate(host, 0);
                swarm_agent_api->DoNewPeerDiscovered(newPeer);
              }
            }
            swarm_agent_api->DoPingSucceeded(host);
          }
          else
          {
            swarm_agent_api->DoPingFailed(host);
          }
        }
        catch (...)
        {
          swarm_agent_api->DoPingFailed(host);
        }
      });
    });

    swarm_agent_api->ToDiscoverBlocks([this, swarm_agent_api, swarm_node, chain_node, nn_core](
                                          const std::string &host, uint32_t count) {
      auto pySwarm = this;
      swarm_node->Post([swarm_agent_api, nn_core, chain_node, host, count, pySwarm]() {
        try
        {
          auto client = nn_core->ConnectTo(host);
          if (!client)
          {
            swarm_agent_api->DoPingFailed(host);
            return;
          }
          auto promised = chain_node->RemoteGetHeaviestChain(count, client);
          if (promised.Wait())
          {
            auto collection = promised.Get();
            if (collection.empty())
            {
              // must get at least genesis or this is an error case.
              swarm_agent_api->DoPingFailed(host);
            }
            bool        loose = false;
            std::string blockId;

            std::string prevHash;
            for (auto &block : collection)
            {
              block.UpdateDigest();
              chain_node->AddBlock(block);
              prevHash = block.prevString();
              loose    = block.loose();
              blockId  = pySwarm->hashToBlockId(block.hash());
              swarm_agent_api->DoNewBlockIdFound(host, blockId);
            }
            if (loose)
            {
              pySwarm->DoLooseBlock(host, prevHash);
            }
          }
          else
          {
            swarm_agent_api->DoPingFailed(host);
          }
        }
        catch (...)
        {
          swarm_agent_api->DoPingFailed(host);
        }
      });
    });

    swarm_agent_api->ToGetBlock([this, swarm_agent_api, swarm_node, chain_node, nn_core](
                                    const std::string &host, const std::string &blockid) {
      auto hashBytes = this->blockIdToHash(blockid);
      auto pySwarm   = this;
      swarm_node->Post(
          [swarm_agent_api, swarm_node, chain_node, nn_core, host, hashBytes, blockid, pySwarm]() {
            try
            {
              auto client = nn_core->ConnectTo(host);
              if (!client)
              {
                swarm_agent_api->DoPingFailed(host);
                return;
              }
              auto promised = chain_node->RemoteGetHeader(hashBytes, client);
              if (promised.Wait())
              {
                auto found = promised.Get().first;
                auto block = promised.Get().second;
                if (found)
                {
                  // add the block to the chain_node.
                  block.UpdateDigest();
                  auto newHash    = block.hash();
                  auto newBlockId = pySwarm->hashToBlockId(newHash);
                  pySwarm->DoBlockSupplied(host, block.hashString());

                  chain_node->AddBlock(block);

                  if (block.loose())
                  {
                    pySwarm->DoLooseBlock(host, block.prevString());
                  }
                }
                else
                {
                  pySwarm->DoBlockNotSupplied(host, blockid);
                }
              }
              else
              {
                pySwarm->DoBlockNotSupplied(host, blockid);
              }
            }
            catch (...)
            {
              swarm_agent_api->DoPingFailed(host);
            }
          });
    });
    swarm_agent_api->ToGetKarma(
        [swarm_node](const std::string &host) { return swarm_node->GetKarma(host); });
    swarm_agent_api->ToAddKarma([swarm_node](const std::string &host, double amount) {
      swarm_node->AddOrUpdate(host, amount);
    });
    swarm_agent_api->ToAddKarmaMax(
        [swarm_node](const std::string &host, double amount, double limit) {
          if (swarm_node->GetKarma(host) < limit)
          {
            swarm_node->AddOrUpdate(host, amount);
          }
        });
    swarm_agent_api->ToGetPeers([swarm_agent_api, swarm_node](uint32_t count, double minKarma) {
      auto                   karmaPeers = swarm_node->GetBestPeers(count, minKarma);
      std::list<std::string> results;
      for (auto &peer : karmaPeers)
      {
        results.push_back(peer.GetLocation().AsString());
      }
      if (results.empty())
      {
        swarm_agent_api->DoPeerless();
      }
      return results;
    });
  }

  virtual ~PySwarm()
  {
    Stop();
  }

#define DELEGATE_CAPTURED , this
#define DELEGATE_WRAPPER                                    \
  std::lock_guard<std::recursive_mutex> lock(this->mutex_); \
  pybind11::gil_scoped_acquire          acquire;
#define DELEGATE swarm_agent_api_->

  /*-------------- MACHINE GENERATED CODE END -------------*/
  virtual void OnIdle(pybind11::object func)
  {
    DELEGATE OnIdle([func DELEGATE_CAPTURED]() { DELEGATE_WRAPPER func(); });
  }
  virtual void OnPeerless(pybind11::object func)
  {
    DELEGATE OnPeerless([func DELEGATE_CAPTURED]() { DELEGATE_WRAPPER func(); });
  }
  virtual void DoPing(const std::string &host)
  {
    DELEGATE DoPing(host);
  }
  virtual void OnPingSucceeded(pybind11::object func)
  {
    DELEGATE OnPingSucceeded(
        [func DELEGATE_CAPTURED](const std::string &host) { DELEGATE_WRAPPER func(host); });
  }
  virtual void OnPingFailed(pybind11::object func)
  {
    DELEGATE OnPingFailed(
        [func DELEGATE_CAPTURED](const std::string &host) { DELEGATE_WRAPPER func(host); });
  }
  virtual void DoDiscoverPeers(const std::string &host, unsigned count)
  {
    DELEGATE DoDiscoverPeers(host, count);
  }
  virtual void OnNewPeerDiscovered(pybind11::object func)
  {
    DELEGATE OnNewPeerDiscovered(
        [func DELEGATE_CAPTURED](const std::string &host) { DELEGATE_WRAPPER func(host); });
  }
  virtual void OnPeerDiscoverFail(pybind11::object func)
  {
    DELEGATE OnPeerDiscoverFail(
        [func DELEGATE_CAPTURED](const std::string &host) { DELEGATE_WRAPPER func(host); });
  }
  virtual void DoBlockSolved(const std::string &blockdata)
  {
    DELEGATE DoBlockSolved(blockdata);
  }
  virtual void DoTransactionListBuilt(const std::list<std::string> &txnlist)
  {
    DELEGATE DoTransactionListBuilt(txnlist);
  }
  virtual void DoDiscoverBlocks(const std::string &host, uint32_t count)
  {
    DELEGATE DoDiscoverBlocks(host, count);
  }
  virtual void OnNewBlockIdFound(pybind11::object func)
  {
    DELEGATE OnNewBlockIdFound(
        [func DELEGATE_CAPTURED](const std::string &host, const std::string &blockid) {
          DELEGATE_WRAPPER func(host, blockid);
        });
  }
  virtual void OnBlockIdRepeated(pybind11::object func)
  {
    DELEGATE OnBlockIdRepeated(
        [func DELEGATE_CAPTURED](const std::string &host, const std::string &blockid) {
          DELEGATE_WRAPPER func(host, blockid);
        });
  }
  virtual void DoGetBlock(const std::string &host, const std::string &blockid)
  {
    DELEGATE DoGetBlock(host, blockid);
  }
  virtual void OnNewBlockAvailable(pybind11::object func)
  {
    DELEGATE OnNewBlockAvailable(
        [func DELEGATE_CAPTURED](const std::string &host, const std::string &blockid) {
          DELEGATE_WRAPPER func(host, blockid);
        });
  }
  virtual std::string GetBlock(const std::string &blockid)
  {
    return DELEGATE GetBlock(blockid);
  }
  virtual void VerifyBlock(const std::string &blockid, bool validity)
  {
    DELEGATE VerifyBlock(blockid, validity);
  }
  virtual void OnNewTxnListIdFound(pybind11::object func)
  {
    DELEGATE OnNewTxnListIdFound(
        [func DELEGATE_CAPTURED](const std::string &host, const std::string &txnlistid) {
          DELEGATE_WRAPPER func(host, txnlistid);
        });
  }
  virtual void DoGetTxnList(const std::string &host, const std::string &txnlistid)
  {
    DELEGATE DoGetTxnList(host, txnlistid);
  }
  virtual void OnNewTxnListAvailable(pybind11::object func)
  {
    DELEGATE OnNewTxnListAvailable(
        [func DELEGATE_CAPTURED](const std::string &host, const std::string &txnlistid) {
          DELEGATE_WRAPPER func(host, txnlistid);
        });
  }
  virtual std::string GetTxnList(const std::string &txnlistid)
  {
    return DELEGATE GetTxnList(txnlistid);
  }
  virtual void AddKarma(const std::string &host, double karma)
  {
    DELEGATE AddKarma(host, karma);
  }
  virtual void AddKarmaMax(const std::string &host, double karma, double limit)
  {
    DELEGATE AddKarmaMax(host, karma, limit);
  }
  virtual double GetKarma(const std::string &host)
  {
    return DELEGATE GetKarma(host);
  }
  virtual double GetCost(const std::string &host)
  {
    return DELEGATE GetCost(host);
  }
  virtual std::list<std::string> GetPeers(uint32_t count, double minKarma)
  {
    return DELEGATE GetPeers(count, minKarma);
  }
  virtual std::string queryOwnLocation()
  {
    return DELEGATE queryOwnLocation();
  }
  /*-------------- MACHINE GENERATED CODE END -------------*/

  std::function<void(const std::string &host, const std::string &blockid)>
               on_new_remote_heaviest_block;
  virtual void DoNewRemoteHeaviestBlock(const std::string &host, const std::string &blockid)
  {
    worker_->Post([this, host, blockid] {
      if (on_new_remote_heaviest_block)
      {
        on_new_remote_heaviest_block(host, blockid);
      }
    });
  }
  virtual void pyOnNewRemoteHeaviestBlock(pybind11::object func)
  {
    OnNewRemoteHeaviestBlock([func, this](const std::string &host, const std::string &blockid) {
      DELEGATE_WRAPPER func(host, blockid);
    });
  }
  virtual void OnNewRemoteHeaviestBlock(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    on_new_remote_heaviest_block = cb;
  }

  std::function<void(const std::string &host, const std::string &blockid)> on_loose_block;
  virtual void DoLooseBlock(const std::string &host, const std::string &blockid)
  {
    worker_->Post([this, host, blockid] {
      if (on_loose_block)
      {
        on_loose_block(host, blockid);
      }
    });
  }
  virtual void PyOnLooseBlock(pybind11::object func)
  {
    OnLooseBlock([func, this](const std::string &host, const std::string &blockid) {
      DELEGATE_WRAPPER func(host, blockid);
    });
  }
  virtual void OnLooseBlock(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    on_loose_block = cb;
  }

  std::function<void(const std::string &host, const std::string &blockid)> on_block_not_supplied;
  virtual void DoBlockNotSupplied(const std::string &host, const std::string &blockid)
  {
    worker_->Post([this, host, blockid] {
      if (on_block_not_supplied)
      {
        on_block_not_supplied(host, blockid);
      }
    });
  }
  virtual void PyOnBlockNotSupplied(pybind11::object func)
  {
    OnBlockNotSupplied([func, this](const std::string &host, const std::string &blockid) {
      DELEGATE_WRAPPER func(host, blockid);
    });
  }
  virtual void OnBlockNotSupplied(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    on_block_not_supplied = cb;
  }

  std::function<void(const std::string &host, const std::string &blockid)> on_block_supplied;
  virtual void DoBlockSupplied(const std::string &host, const std::string &blockid)
  {
    worker_->Post([this, host, blockid] {
      if (on_block_supplied)
      {
        on_block_supplied(host, blockid);
      }
    });
  }
  virtual void PyOnBlockSupplied(pybind11::object func)
  {
    OnBlockSupplied([func, this](const std::string &host, const std::string &blockid) {
      DELEGATE_WRAPPER func(host, blockid);
    });
  }
  virtual void OnBlockSupplied(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    on_block_supplied = cb;
  }

  std::string HeaviestBlock() const
  {
    return hashToBlockId(chain_node_->HeaviestBlock().hash());
  }

private:
  std::shared_ptr<PythonWorker>                                  worker_;
  std::shared_ptr<fetch::network::NetworkNodeCore>               nn_core_;
  std::shared_ptr<fetch::swarm::SwarmAgentApiImpl<PythonWorker>> swarm_agent_api_;
  std::shared_ptr<fetch::swarm::SwarmHttpModule>                 http_module_;
  std::shared_ptr<fetch::swarm::SwarmNode>                       swarm_node_;
  std::shared_ptr<fetch::swarm::SwarmRandom>                     rnd_;
  std::shared_ptr<fetch::ledger::MainChainNode>                  chain_node_;
  mutex_type                                                     mutex_;
};

}  // namespace swarm
}  // namespace fetch
