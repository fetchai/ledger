#pragma once

#include <utility>

#include "network/details/thread_pool.hpp"
#include "network/swarm/swarm_agent_api.hpp"
#include "network/swarm/swarm_karma_peers.hpp"

namespace fetch {
namespace swarm {

template <class threading_system_type>
class SwarmAgentApiImpl : public SwarmAgentApi
{
public:
  SwarmAgentApiImpl(SwarmAgentApiImpl &rhs)  = delete;
  SwarmAgentApiImpl(SwarmAgentApiImpl &&rhs) = delete;
  SwarmAgentApiImpl operator=(SwarmAgentApiImpl &rhs) = delete;
  SwarmAgentApiImpl operator=(SwarmAgentApiImpl &&rhs) = delete;

  std::shared_ptr<threading_system_type> threadingSystem_;
  std::string                            identifier_;
  uint32_t                               idlespeed_;

  explicit SwarmAgentApiImpl(std::string identifier, uint32_t idlespeed)
    : identifier_(std::move(identifier)), idlespeed_(idlespeed)
  {
    threadingSystem_ = std::make_shared<threading_system_type>(10);
  }

  explicit SwarmAgentApiImpl(std::shared_ptr<threading_system_type> threadingSystem,
                             std::string identifier, uint32_t idlespeed)
    : identifier_(std::move(identifier)), idlespeed_(idlespeed)
  {
    threadingSystem_ = threadingSystem;
  }

  virtual std::string queryOwnLocation() { return identifier_; }

  void Start()
  {
    threadingSystem_->Start();
    startIdle();
  }

  void Stop() { threadingSystem_->Stop(); }

  virtual ~SwarmAgentApiImpl()
  {
    threadingSystem_->Stop();
    threadingSystem_.reset();
  }

  void startIdle()
  {
    auto lambd = [this] { this->DoIdle(); };
    threadingSystem_->Post(lambd);
  }

  int idleCount = 0;

  void DoIdle()
  {
    idleCount++;
    if (this->onIdle_)
    {
      try
      {
        this->onIdle_();
      }
      catch (std::exception &x)
      {
        std::cerr << "SwarmAgentApiImpl::DoIdle Exception ignored:" << x.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << "SwarmAgentApiImpl::DoIdle Exception ignored." << std::endl;
      }
    }
    auto lambd = [this] { this->DoIdle(); };
    threadingSystem_->Post(lambd, idlespeed_);
  }

  virtual void OnIdle(std::function<void()> cb) { onIdle_ = cb; }

  virtual void OnPeerless(std::function<void()> cb) { onPeerless_ = cb; }

  virtual void DoPing(const std::string &host)
  {
    threadingSystem_->Post([this, host] {
      if (this->toPing_)
      {
        this->toPing_(*this, host);
      }
    });
  }

  virtual void DoPeerless()
  {
    threadingSystem_->Post([this] {
      if (this->onPeerless_)
      {
        this->onPeerless_();
      }
    });
  }

  virtual void ToPing(std::function<void(SwarmAgentApi &api, const std::string &host)> action)
  {
    toPing_ = action;
  }

  virtual void OnPingSucceeded(std::function<void(const std::string &host)> cb)
  {
    onPingSucceeded_ = cb;
  }

  virtual void OnPingFailed(std::function<void(const std::string &host)> cb) { onPingFailed_ = cb; }

  virtual void DoPingSucceeded(const std::string &host)
  {
    threadingSystem_->Post([this, host] {
      if (this->onPingSucceeded_)
      {
        this->onPingSucceeded_(host);
      }
    });
  }

  virtual void DoPingFailed(const std::string &host)
  {
    threadingSystem_->Post([this, host] {
      if (this->onPingFailed_)
      {
        this->onPingFailed_(host);
      }
    });
  }

  virtual void DoDiscoverPeers(const std::string &host, uint32_t count)
  {
    threadingSystem_->Post([this, host, count] {
      if (this->toDiscoverPeers_)
      {
        this->toDiscoverPeers_(*this, host, count);
      }
    });
  }

  virtual void ToDiscoverPeers(
      std::function<void(SwarmAgentApi &api, const std::string &host, uint32_t count)> action)
  {
    toDiscoverPeers_ = action;
  }

  virtual void OnNewPeerDiscovered(std::function<void(const std::string &host)> cb)
  {
    onNewPeerDiscovered_ = cb;
  }

  virtual void DoNewPeerDiscovered(const std::string &host)
  {
    threadingSystem_->Post([this, host] {
      if (this->onNewPeerDiscovered_)
      {
        this->onNewPeerDiscovered_(host);
      }
    });
  }

  virtual void OnPeerDiscoverFail(std::function<void(const std::string &host)> cb)
  {
    onPeerDiscoverFail_ = cb;
  }

  virtual void ToBlockSolved(std::function<void(const std::string &blockdata)> action)
  {
    this->toBlockSolved_ = action;
  }

  virtual void DoBlockSolved(const std::string &blockdata)
  {
    if (this->toBlockSolved_)
    {
      this->toBlockSolved_(blockdata);
    }
  }

  virtual void DoDiscoverBlocks(const std::string &host, uint32_t count)
  {
    if (this->toDiscoverBlocks_)
    {
      this->toDiscoverBlocks_(host, count);
    }
  }

  virtual void ToDiscoverBlocks(std::function<void(const std::string &host, uint32_t count)> action)
  {
    this->toDiscoverBlocks_ = action;
  }

  // Announce new block IDs to the agent.

  virtual void DoNewBlockIdFound(const std::string &host, const std::string &blockid)
  {
    threadingSystem_->Post([this, host, blockid] {
      if (this->onNewBlockIdFound_)
      {
        this->onNewBlockIdFound_(host, blockid);
      }
    });
  }

  virtual void OnNewBlockIdFound(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    onNewBlockIdFound_ = cb;
  }

  virtual void OnBlockIdRepeated(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    onBlockIdRepeated_ = cb;
  }

  virtual void DoBlockIdRepeated(const std::string &host, const std::string &blockid)
  {
    threadingSystem_->Post([this, host, blockid] {
      if (this->onBlockIdRepeated_)
      {
        this->onBlockIdRepeated_(host, blockid);
      }
    });
  }

  // Verb for pulling blocks over network.

  virtual void ToGetBlock(
      std::function<void(const std::string &host, const std::string &blockid)> action)
  {
    toGetBlock_ = action;
  }

  virtual void ToQueryBlock(std::function<std::string(const std::string &blockid)> action)
  {
    toQueryBlock_ = action;
  }

  virtual void DoGetBlock(const std::string &host, const std::string &blockid)
  {
    if (this->toGetBlock_)
    {
      this->toGetBlock_(host, blockid);
    }
  }

  // Announce new blocks to the agent.

  virtual void OnNewBlockAvailable(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    onNewBlockAvailable_ = cb;
  }

  virtual void DoNewBlockAvailable(const std::string &host, const std::string &blockid)
  {
    threadingSystem_->Post([this, host, blockid] {
      if (this->onNewBlockAvailable_)
      {
        this->onNewBlockAvailable_(host, blockid);
      }
    });
  }

  // Get block from local cache.

  virtual std::string GetBlock(const std::string &blockid)
  {
    if (this->toQueryBlock_)
    {
      return this->toQueryBlock_(blockid);
    }
    return "";
  }

  virtual void ToVerifyBlock(std::function<void(const std::string &blockid, bool validity)> action)
  {
    toVerifyBlock_ = action;
  }

  virtual void VerifyBlock(const std::string &blockid, bool validity)
  {
    if (this->toVerifyBlock_)
    {
      this->toVerifyBlock_(blockid, validity);
    }
  }

  // HANDLE TXN LIST TRANSMISSIONS -------------------------------------xs

  virtual void DoTransactionListBuilt(const std::list<std::string> &txnlist) {}

  // TODO(katie) Implement below.

  virtual void OnNewTxnListIdFound(
      std::function<void(const std::string &host, const std::string &txnlistid)> cb)
  {
    onNewTxnListIdFound_ = cb;
  }

  virtual void DoGetTxnList(const std::string &host, const std::string &txnlistid) {}

  virtual void OnNewTxnListAvailable(
      std::function<void(const std::string &host, const std::string &blockid)> cb)
  {
    onNewTxnListAvailable_ = cb;
  }

  virtual std::string GetTxnList(const std::string &txnlistid) { return "[]"; }

  virtual void ToGetKarma(std::function<double(const std::string &host)> query)
  {
    this->toGetKarma_ = query;
  }
  virtual double GetKarma(const std::string &host)
  {
    if (this->toGetKarma_)
    {
      return toGetKarma_(host);
    }
    return 0.0;
  }

  virtual void ToAddKarma(std::function<void(const std::string &host, double amount)> action)
  {
    this->toAddKarma_ = action;
  }
  virtual void AddKarma(const std::string &host, double amount)
  {
    if (this->toAddKarma_)
    {
      toAddKarma_(host, amount);
    }
  }

  virtual void ToGetPeers(
      std::function<std::list<std::string>(uint32_t count, double minKarma)> query)
  {
    this->toGetPeers_ = query;
  }

  virtual std::list<std::string> GetPeers(uint32_t count, double minKarma)
  {
    if (this->toGetPeers_)
    {
      return toGetPeers_(count, minKarma);
    }
    return std::list<std::string>();
  }

  virtual void ToAddKarmaMax(
      std::function<void(const std::string &host, double karma, double limit)> action)
  {
    this->toAddKarmaMax_ = action;
  }
  virtual void AddKarmaMax(const std::string &host, double karma, double limit)
  {
    if (this->toAddKarmaMax_)
    {
      return toAddKarmaMax_(host, karma, limit);
    }
  }

  virtual double GetCost(const std::string &host) { return 1.0; }

protected:
  std::function<void()> onIdle_;
  std::function<void()> onPeerless_;

  std::function<void(const std::string &host)>                               onPingSucceeded_;
  std::function<void(const std::string &host)>                               onPingFailed_;
  std::function<void(const std::string &host)>                               onNewPeerDiscovered_;
  std::function<void(const std::string &host)>                               onPeerDiscoverFail_;
  std::function<void(const std::string &host, const std::string &blockid)>   onNewBlockIdFound_;
  std::function<void(const std::string &host, const std::string &blockid)>   onBlockIdRepeated_;
  std::function<void(const std::string &host, const std::string &blockid)>   onNewBlockAvailable_;
  std::function<void(const std::string &host, const std::string &txnlistid)> onNewTxnListIdFound_;
  std::function<void(const std::string &host, const std::string &txnlistid)> onNewTxnListAvailable_;

  std::function<void(SwarmAgentApi &api, const std::string &host)>                 toPing_;
  std::function<void(SwarmAgentApi &api, const std::string &host, uint32_t count)> toDiscoverPeers_;

  std::function<void(const std::string &blockdata)>                        toBlockSolved_;
  std::function<void(const std::string &host, const std::string &blockid)> toGetBlock_;
  std::function<void(const std::string &host, uint32_t count)>             toDiscoverBlocks_;
  std::function<std::string(const std::string &blockid)>                   toQueryBlock_;

  std::function<double(const std::string &host)>                           toGetKarma_;
  std::function<void(const std::string &host, double amount)>              toAddKarma_;
  std::function<std::list<std::string>(uint32_t count, double minKarma)>   toGetPeers_;
  std::function<void(const std::string &host, double karma, double limit)> toAddKarmaMax_;
  std::function<void(const std::string &blockid, bool validity)>           toVerifyBlock_;
};
}  // namespace swarm
}  // namespace fetch
