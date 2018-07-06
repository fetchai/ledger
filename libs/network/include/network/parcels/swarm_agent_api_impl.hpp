#ifndef SWARM_AGENT_API_IMPL__
#define SWARM_AGENT_API_IMPL__

#include "network/swarm/swarm_agent_api.hpp"
#include "network/swarm/swarm_karma_peers.hpp"
#include "network/details/thread_pool.hpp"

namespace fetch
{
namespace swarm
{

template<class threading_system_type = network::details::ThreadPoolImplementation>
class SwarmAgentApiImpl:public SwarmAgentApi
{
public:
  SwarmAgentApiImpl(SwarmAgentApiImpl &rhs)            = delete;
  SwarmAgentApiImpl(SwarmAgentApiImpl &&rhs)           = delete;
  SwarmAgentApiImpl operator=(SwarmAgentApiImpl &rhs)  = delete;
  SwarmAgentApiImpl operator=(SwarmAgentApiImpl &&rhs) = delete;

  std::shared_ptr<threading_system_type> threadingSystem_;
  std::string identifier_;
  unsigned int idlespeed_;

  explicit SwarmAgentApiImpl(const std::string &identifier, unsigned int idlespeed):identifier_(identifier), idlespeed_(idlespeed)
  {
    threadingSystem_ = std::make_shared<threading_system_type>(10);
  }

  explicit SwarmAgentApiImpl(std::shared_ptr<threading_system_type> threadingSystem, const std::string &identifier, unsigned int idlespeed):identifier_(identifier), idlespeed_(idlespeed)
  {
    threadingSystem_ = threadingSystem;
  }

  virtual std::string queryOwnLocation()
  {
    return identifier_;
  }

  void Start()
  {
    threadingSystem_ -> Start();
    startIdle();
  }

  void Stop()
  {
    threadingSystem_ -> Stop();
  }

  virtual ~SwarmAgentApiImpl()
  {
    threadingSystem_ -> Stop();
    threadingSystem_.reset();
  }

  void startIdle()
  {
    auto lambd = [this]
      {
        this->DoIdle();
      };
    threadingSystem_ -> Post(lambd);
  }

  int idleCount = 0;

  void DoIdle()
  {
    idleCount++;
    if (this->onIdle_)
      {
        try
          {
            cout << ">>>>>>>>>>>>>>>>>>> ON IDLE " << idleCount<<endl;
            this->onIdle_();
            cout << ">>>>>>>>>>>>>>>>>>> DONE IDLE " << idleCount<<endl;
          }
        catch (std::exception &x)
          {
            cerr << "SwarmAgentApiImpl::DoIdle Exception ignored:" << x.what() << endl;
          }
        catch(...)
          {
            cerr << "SwarmAgentApiImpl::DoIdle Exception ignored." << endl;
          }
      }
    auto lambd = [this]
      {
        this->DoIdle();
      };
    threadingSystem_ -> Post(lambd, int(idlespeed_));
  }

  virtual void OnIdle                (std::function<void ()> cb)
  {
    onIdle_ = cb;
    cout << ">>>>>>>>>>>>>>>>>>> ON IDLE SET"<<endl;
  }

  virtual void OnPeerless            (std::function<void ()> cb)
  {
    onPeerless_ = cb;
  }

  virtual void DoPing                (const std::string &host)
  {
    threadingSystem_ -> Post([this, host]{
        if (this->toPing_)
          {
            this->toPing_(std::ref(*this), std::cref(host));
          }
      });
  }

  virtual void DoPeerless            ()
  {
    threadingSystem_ -> Post([this]{
        if (this->onPeerless_)
          {
            this->onPeerless_();
          }
      });
  }

  virtual void ToPing                (std::function<void (SwarmAgentApi &api, const std::string &host)> action)
  {
    toPing_ = action;
  }

  virtual void OnPingSucceeded       (std::function<void (const std::string &host)> cb)
  {
    onPingSucceeded_ = cb;
  }

  virtual void OnPingFailed          (std::function<void (const std::string &host)> cb)
  {
    onPingFailed_ = cb;
  }

  virtual void DoPingSucceeded       (const std::string &host)
  {
    threadingSystem_ -> Post([this, host]{
        if (this->onPingSucceeded_)
          {
            this->onPingSucceeded_(std::cref(host));
          }
      });
  }

  virtual void DoPingFailed          (const std::string &host)
  {
    threadingSystem_ -> Post([this, host]{
        if (this->onPingFailed_)
          {
            this->onPingFailed_(std::cref(host));
          }
      });
  }

  virtual void DoDiscoverPeers       (const std::string &host, unsigned int count)
  {
    threadingSystem_ -> Post([this, host, count]{
        if (this->toDiscoverPeers_)
          {
            this->toDiscoverPeers_(std::ref(*this), std::cref(host), count);
          }
      });
  }

  virtual void ToDiscoverPeers       (std::function<void (SwarmAgentApi &api, const std::string &host, unsigned int count)> action)
  {
    toDiscoverPeers_ = action;
  }

  virtual void OnNewPeerDiscovered   (std::function<void (const std::string &host)> cb)
  {
    onNewPeerDiscovered_ = cb;
  }

  virtual void DoNewPeerDiscovered   (const std::string &host)
  {
    threadingSystem_ -> Post([this, host]{
        if (this->onNewPeerDiscovered_)
          {
            this->onNewPeerDiscovered_(std::cref(host));
          }
      });
  }

  virtual void OnPeerDiscoverFail    (std::function<void (const std::string &host)> cb)
  {
    onPeerDiscoverFail_ = cb;
  }

  // HANDLE BLOCK TRANSMISSIONS ------------------------------------------------------------------------

  virtual void ToBlockSolved         (std::function<void (const std::string &blockdata)> action)
  {
    this -> toBlockSolved_ = action;
  }

  virtual void DoBlockSolved         (const std::string &blockdata)
  {
    if (this -> toBlockSolved_)
      {
        this -> toBlockSolved_(std::cref(blockdata));
      }
  }

  virtual void DoDiscoverBlocks       (const std::string &host, unsigned int count)
  {
    if (this->toDiscoverBlocks_)
      {
        this->toDiscoverBlocks_(std::cref(host), count);
      }
  }

  virtual void ToDiscoverBlocks         (std::function<void (const std::string &host, unsigned int count)> action)
  {
    this->toDiscoverBlocks_ = action;
  }

  // Announce new block IDs to the agent.

  virtual void DoNewBlockIdFound   (const std::string &host, const std::string &blockid)
  {
    threadingSystem_ -> Post([this, host, blockid]{
        if (this->onNewBlockIdFound_)
          {
            this->onNewBlockIdFound_(std::cref(host), std::cref(blockid));
          }
      });
  }

  virtual void OnNewBlockIdFound     (std::function<void (const std::string &host, const std::string &blockid)> cb)
  {
    onNewBlockIdFound_ = cb;
  }

  virtual void OnBlockIdRepeated  (std::function<void (const std::string &host, const std::string &blockid)> cb)
  {
    onBlockIdRepeated_ = cb;
  }

  virtual void DoBlockIdRepeated   (const std::string &host, const std::string &blockid)
  {
    threadingSystem_ -> Post([this, host, blockid]{
        if (this -> onBlockIdRepeated_)
          {
            this -> onBlockIdRepeated_(host, blockid);
          }
      });
  }

  // Verb for pulling blocks over network.

  virtual void ToGetBlock     (std::function<void (const std::string &host, const std::string &blockid)> action)
  {
    toGetBlock_ = action;
  }

  virtual void ToQueryBlock     (std::function<std::string (const std::string &blockid)> action)
  {
    toQueryBlock_ = action;
  }

  virtual void DoGetBlock            (const std::string &host, const std::string &blockid)
  {
    if (this->toGetBlock_)
      {
        this->toGetBlock_(std::cref(host), std::cref(blockid));
      }
  }

  // Announce new blocks to the agent.

  virtual void OnNewBlockAvailable   (std::function<void (const std::string &host, const std::string &blockid)> cb)
  {
    onNewBlockAvailable_ = cb;
  }

  virtual void DoNewBlockAvailable   (const std::string &host, const std::string &blockid)
  {
    threadingSystem_ -> Post([this, host, blockid]{
        if (this -> onNewBlockAvailable_)
          {
            this -> onNewBlockAvailable_(host, blockid);
          }
      });
  }

  // Get block from local cache.

  virtual std::string GetBlock       (const std::string &blockid)
  {
    if (this->toQueryBlock_)
      {
        return this->toQueryBlock_(std::cref(blockid));
      }
    return "";
  }

  virtual void ToVerifyBlock     (std::function<void (const std::string &blockid, bool validity)> action)
  {
    toVerifyBlock_ = action;
  }

  virtual void VerifyBlock               (const std::string &blockid, bool validity)
  {
    if (this->toVerifyBlock_)
      {
        this->toVerifyBlock_(std::cref(blockid), validity);
      }
  }

  // HANDLE TXN LIST TRANSMISSIONS ---------------------------------------------------------------------

  virtual void DoTransactionListBuilt(const std::list<std::string> &txnlist)
  {
  }

  // TODO(katie) Implement below.

  virtual void OnNewTxnListIdFound   (std::function<void (const std::string &host, const std::string &txnlistid)> cb)
  {
    onNewTxnListIdFound_ = cb;
  }

  virtual void DoGetTxnList          (const std::string &host, const std::string &txnlistid)
  {
  }

  virtual void OnNewTxnListAvailable (std::function<void (const std::string &host, const std::string &blockid)> cb)
  {
    onNewTxnListAvailable_ = cb;
  }

  virtual std::string GetTxnList     (const std::string &txnlistid)
  {
    return "[]";
  }

  virtual void ToGetKarma(std::function<double (const std::string &host)> query) { this->toGetKarma_ = query; }
  virtual double GetKarma                  (const std::string &host) { if (this->toGetKarma_) { return toGetKarma_(host); } return 0.0; }

  virtual void ToAddKarma(std::function<void (const std::string &host, double amount)> action) { this->toAddKarma_ = action; }
  virtual void AddKarma                  (const std::string &host, double amount) { if (this->toAddKarma_) { toAddKarma_(host, amount); } }

  virtual void  ToGetPeers(std::function<std::list<std::string>(unsigned int count, double minKarma)> query) { this->toGetPeers_ = query; }
  virtual std::list<std::string> GetPeers(unsigned int count, double minKarma) { if (this->toGetPeers_) { return toGetPeers_(count, minKarma); } return std::list<std::string>(); }

  virtual void ToAddKarmaMax(std::function<void(const std::string &host, double karma, double limit)> action) { this->toAddKarmaMax_ = action; }
  virtual void AddKarmaMax(const std::string &host, double karma, double limit) { if (this->toAddKarmaMax_) { return toAddKarmaMax_(host, karma, limit); } }


  virtual double GetCost             (const std::string &host)
  {
    return 1.0;
  }

protected:
  std::function<void ()> onIdle_;
  std::function<void ()> onPeerless_;

  std::function<void (const std::string &host)> onPingSucceeded_;
  std::function<void (const std::string &host)> onPingFailed_;
  std::function<void (const std::string &host)> onNewPeerDiscovered_;
  std::function<void (const std::string &host)> onPeerDiscoverFail_;
  std::function<void (const std::string &host, const std::string &blockid)> onNewBlockIdFound_;
  std::function<void (const std::string &host, const std::string &blockid)> onBlockIdRepeated_;
  std::function<void (const std::string &host, const std::string &blockid)> onNewBlockAvailable_;
  std::function<void (const std::string &host, const std::string &txnlistid)> onNewTxnListIdFound_;
  std::function<void (const std::string &host, const std::string &txnlistid)> onNewTxnListAvailable_;

  std::function<void (SwarmAgentApi &api, const std::string &host)> toPing_;
  std::function<void (SwarmAgentApi &api, const std::string &host, unsigned int count)> toDiscoverPeers_;

  std::function<void(const std::string &blockdata)> toBlockSolved_;
  std::function<void(const std::string &host, const std::string &blockid)> toGetBlock_;
  std::function<void (const std::string &host, unsigned int count)> toDiscoverBlocks_;
  std::function<std::string (const std::string &blockid)> toQueryBlock_;

  std::function<double (const std::string &host)>                            toGetKarma_;
  std::function<void (const std::string &host, double amount)>               toAddKarma_;
  std::function<std::list<std::string>(unsigned int count, double minKarma)> toGetPeers_;
  std::function<void(const std::string &host, double karma, double limit)>   toAddKarmaMax_;
  std::function<void (const std::string &blockid, bool validity)> toVerifyBlock_;
};
}
}

#endif //__SWARM_AGENT_API_IMPL__
