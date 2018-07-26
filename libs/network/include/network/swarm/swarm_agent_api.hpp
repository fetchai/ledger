#pragma once

#include <string>
#include <list>
#include <functional>

namespace fetch
{
namespace swarm
{

class SwarmAgentApi
{
public:
  SwarmAgentApi() = default;
  virtual ~SwarmAgentApi() = default;

  virtual void OnIdle                    (std::function<void ()> cb) = 0;
  virtual void OnPeerless                (std::function<void ()> cb) = 0;

  virtual void DoPing                    (const std::string &host) = 0;
  virtual void OnPingSucceeded           (std::function<void (const std::string &host)> cb) = 0;
  virtual void OnPingFailed              (std::function<void (const std::string &host)> cb) = 0;

  virtual void DoDiscoverPeers           (const std::string &host, unsigned count) = 0;
  virtual void OnNewPeerDiscovered       (std::function<void (const std::string &host)> cb) = 0;
  virtual void OnPeerDiscoverFail        (std::function<void (const std::string &host)> cb) = 0;

  virtual void DoBlockSolved             (const std::string &blockdata) = 0;
  virtual void DoTransactionListBuilt(const std::list<std::string> &txnlist) = 0;

  virtual void DoDiscoverBlocks          (const std::string &host, uint32_t count) = 0;
  virtual void OnNewBlockIdFound         (std::function<void (const std::string &host, const std::string &blockid)> cb) = 0;
  virtual void OnBlockIdRepeated  (std::function<void (const std::string &host, const std::string &blockid)> cb) = 0;
  virtual void DoGetBlock                (const std::string &host, const std::string &blockid) = 0;
  virtual void OnNewBlockAvailable       (std::function<void (const std::string &host, const std::string &blockid)> cb) = 0;
  virtual std::string GetBlock           (const std::string &blockid) = 0;

  virtual void VerifyBlock               (const std::string &blockid, bool validity)=0;

  virtual void OnNewTxnListIdFound       (std::function<void (const std::string &host, const std::string &txnlistid)> cb) = 0;
  virtual void DoGetTxnList              (const std::string &host, const std::string &txnlistid) = 0;
  virtual void OnNewTxnListAvailable     (std::function<void (const std::string &host, const std::string &txnlistid)> cb) = 0;
  virtual std::string GetTxnList         (const std::string &txnlistid) = 0;

  virtual void AddKarma                  (const std::string &host, double karma) = 0;
  virtual void AddKarmaMax               (const std::string &host, double karma, double limit) = 0;
  virtual double GetKarma                (const std::string &host) = 0;
  virtual double GetCost                 (const std::string &host) = 0;
  virtual std::list<std::string> GetPeers(uint32_t count, double minKarma) = 0;

  virtual std::string queryOwnLocation   () = 0;

  SwarmAgentApi(SwarmAgentApi &rhs)            = delete;
  SwarmAgentApi(SwarmAgentApi &&rhs)           = delete;
};

}
}

