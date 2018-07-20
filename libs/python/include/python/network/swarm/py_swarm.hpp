#ifndef __PY_SWARM_HPP
#define __PY_SWARM_HPP

#include <iostream>
#include <string>
#include <iostream>

#include "core/commandline/parameter_parser.hpp"
#include "network/swarm/swarm_node.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "network/swarm/swarm_agent_api_impl.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "python/worker/python_worker.hpp"
#include "ledger/main_chain_node.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/byte_array/decoders.hpp"
#include "network/protocols/mainchain/commands.hpp"
#include"network/service/function.hpp"

#include <unistd.h>
#include <string>
#include <time.h>

namespace fetch
{
namespace swarm
{

class PySwarm
{
public:
    typedef fetch::ledger::MainChainNode::block_type block_type;
    typedef fetch::service::protocol_handler_type protocol_handler_type;
 
  PySwarm(const PySwarm &rhs)
  {
    nnCore_        = rhs.nnCore_;
    rnd_           = rhs.rnd_;
    swarmAgentApi_ = rhs.swarmAgentApi_;
    swarmNode_     = rhs.swarmNode_;
    worker_        = rhs.worker_;
    chainNode_     = rhs.chainNode_;
  }
  PySwarm(PySwarm &&rhs)
  {
    nnCore_        = std::move(rhs.nnCore_);
    rnd_           = std::move(rhs.rnd_);
    swarmAgentApi_ = std::move(rhs.swarmAgentApi_);
    swarmNode_     = std::move(rhs.swarmNode_);
    worker_        = std::move(rhs.worker_);
    chainNode_     = std::move(rhs.chainNode_);
  }
  PySwarm operator=(const PySwarm &rhs)  = delete;
  PySwarm operator=(PySwarm &&rhs) = delete;
  bool operator==(const PySwarm &rhs) const = delete;
  bool operator<(const PySwarm &rhs) const = delete;

  typedef std::recursive_mutex mutex_type;
  typedef std::lock_guard<std::recursive_mutex>lock_type;
  mutex_type mutex_;

  virtual void Start()
  {
    lock_type lock(mutex_);
    swarmAgentApi_ -> Start();
    nnCore_ -> Start();
    chainNode_ -> StartMining();
  }

  virtual void Stop()
  {
    lock_type lock(mutex_);
    nnCore_ -> Stop();
    swarmAgentApi_ -> Stop();
  }

  std::shared_ptr<PythonWorker> worker_;
  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;
  std::shared_ptr<fetch::swarm::SwarmAgentApiImpl<PythonWorker>> swarmAgentApi_;
  std::shared_ptr<fetch::swarm::SwarmHttpModule> httpModule_;
  std::shared_ptr<fetch::swarm::SwarmNode> swarmNode_;
  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;

  std::shared_ptr<fetch::ledger::MainChainNode> chainNode_;

  fetch::chain::MainChain::block_hash blockIdToHash(const std::string &id) const
  {
    return fetch::byte_array::FromHex(id.c_str());
  }

  std::string hashToBlockId(const fetch::chain::MainChain::block_hash &hash) const
  {
    return std::string(fetch::byte_array::ToHex(hash));
  }

    class Discovery
    {
    public:
        std::string host_;
        std::shared_ptr<network::NetworkNodeCore::Subscription> subs_;
        Discovery(std::string host, std::shared_ptr<network::NetworkNodeCore::Subscription> subs)
        {
            subs_ = subs;
            host_ = host;
        }
    };

    std::map<std::string, std::shared_ptr<Discovery>> disc;

  explicit PySwarm(uint32_t id, uint16_t rpcPort, uint16_t httpPort, uint32_t maxpeers, uint32_t idlespeed, int target, int chainident)
  {
    std::string identifier = "node-" + std::to_string(id);
    std::string myHost = "127.0.0.1:" + std::to_string(rpcPort);
    fetch::swarm::SwarmPeerLocation myHostLoc(myHost);

    auto worker = std::make_shared<PythonWorker>();
    auto nnCore = std::make_shared<fetch::network::NetworkNodeCore>(40, httpPort, rpcPort);
    auto rnd = std::make_shared<fetch::swarm::SwarmRandom>(id);
    auto swarmNode = std::make_shared<fetch::swarm::SwarmNode>(nnCore, identifier, maxpeers, myHost);

    auto httpModule = std::make_shared<SwarmHttpModule>(swarmNode);
    nnCore -> AddModule(httpModule);

    auto chainNode = std::make_shared<fetch::ledger::MainChainNode>(nnCore, id, target, chainident);
    auto swarmAgentApi = std::make_shared<fetch::swarm::SwarmAgentApiImpl<PythonWorker>>(worker, myHost, idlespeed);
    worker -> UseCore(nnCore);

    auto chain = std::make_shared<fetch::ledger::MainChain>();


    fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([](){ return time(0); });

    httpModule_ = httpModule;
    nnCore_ = nnCore;
    rnd_ = rnd;
    swarmAgentApi_ = swarmAgentApi;
    swarmNode_ = swarmNode;
    worker_ = worker;
    chainNode_ = chainNode;

    nnCore_ -> Start();

    // TODO(kll) Move this setup code somewhere more sensible.
    swarmAgentApi -> ToPing([swarmAgentApi, nnCore, swarmNode](fetch::swarm::SwarmAgentApi &unused, const std::string &host)
                            {
                                swarmNode -> Post([swarmAgentApi, swarmNode, nnCore, host]()
                                                  {
                                                      try
                                                      {
                                          auto cli = nnCore -> ConnectTo(host);
                                          if (!cli)
                                          {
                                               fetch::logger.Info("AGENT_API SUBSCRIBE TIMEOUT ", host );
                                               swarmAgentApi -> DoPingFailed(host);
                                               return;
                                          }
                                                          auto newPeer = swarmNode -> AskPeerForPeers(host, cli);
                                                          if (newPeer.length())
                                                          {
                                                              if (!swarmNode -> IsOwnLocation(newPeer))
                                                              {
                                                                  if (!swarmNode -> IsExistingPeer(newPeer))
                                                                  {
                                                                      swarmNode -> AddOrUpdate(host, 0);
                                                                      swarmAgentApi -> DoNewPeerDiscovered(newPeer);
                                                                  }
                                                              }
                                                              swarmAgentApi -> DoPingSucceeded(host);
                                                          }
                                                          else
                                                          {
                                                              swarmAgentApi -> DoPingFailed(host);
                                                          }
                                                      } catch (...)
                                                      {
                                                          swarmAgentApi -> DoPingFailed(host);
                                                      }
                                                  });
                            });

    chainNode -> onBlockComplete_ = [chainNode](const block_type blk){
        fetch::logger.Info("AGENT_API PUBLISHING ", blk.summarise() );

        chainNode ->  Publish(fetch::ledger::MainChain::BLOCK_PUBLISH, blk);
    };

    

    swarmAgentApi -> ToDiscoverBlocks([this, swarmAgentApi, swarmNode, chainNode, nnCore](const std::string &host, uint32_t count)
                                      {
                                          if (disc.find(host) != disc.end())
                                          {
                                              fetch::logger.Info("AGENT_API UNSUBSCRIBE ", host );
                                              disc.erase(host);
                                          }
                                      });


                                      
    swarmAgentApi -> ToDiscoverBlocks([this, swarmAgentApi, swarmNode, chainNode, nnCore](const std::string &host, uint32_t count)
                                       {
                                      auto pySwarm = this;

                                      if (disc.find(host) == disc.end())
                                      {
                                          auto func = new service::Function<void(const block_type)>
                                              (
                                               [pySwarm,host,chainNode,swarmAgentApi](const block_type &blk){
                                                   block_type block = blk;
                                                   fetch::logger.Info("AGENT_API DISCOVER WAS SENT ", block.summarise(), " from ", host );
                   
                                                   block.UpdateDigest();
                                                   auto newblock = chainNode -> AddBlock(block);

                                                   if (newblock)
                                                   {
                                                       fetch::logger.Info("AGENT_API DISCOVER GOT ", block.summarise(),  (newblock?" NEW":" OLD"), " from ", host );
                                                       swarmAgentApi -> DoNewBlockIdFound(host, block.hashString());
                                                       if (block.loose())
                                                       {
                                                           fetch::logger.Info("AGENT_API DISCOVER LOOSE ", block.prevString() );
                                                           pySwarm -> DoLooseBlock(host, block.prevString());
                                                       }
                                                       fetch::logger.Info("AGENT_API DISCOVER FORWARDING", block.summarise() );
                                                       chainNode ->  Publish(fetch::ledger::MainChain::BLOCK_PUBLISH, block);
                                                   }
                                                   else
                                                   {
                                                       fetch::logger.Info("AGENT_API DISCOVER WAS DUPE ", block.summarise(), " from ", host );
                                                   }

                                               }
                                               );
                                          auto cli = nnCore -> ConnectTo(host);
                                          if (!cli)
                                          {
                                               fetch::logger.Info("AGENT_API SUBSCRIBE TIMEOUT ", host );
                                               swarmAgentApi -> DoPingFailed(host);
                                               return;
                                          }

                                          auto subs = nnCore ->
                                              CreateSubscription(cli,
                                                                 fetch::protocols::FetchProtocols::MAIN_CHAIN,
                                                                 fetch::ledger::MainChain::BLOCK_PUBLISH,
                                                                 func
                                                                 );
                                          fetch::logger.Info("AGENT_API SUBSCRIBE ", host );
                                          this -> disc[host] = std::make_shared<Discovery>(host, subs);
                                      }
                                     });

    swarmAgentApi -> ToLoadChain([this, swarmAgentApi, swarmNode, chainNode, nnCore](const std::string &host, uint32_t count)
                                       {
                                      auto pySwarm = this;
                                      swarmNode ->Post([swarmAgentApi, nnCore, chainNode, host, count, pySwarm]()
                                                       {
                                                           try
                                                           {
                                          auto cli = nnCore -> ConnectTo(host);
                                          if (!cli)
                                          {
                                               fetch::logger.Info("AGENT_API LOADCHAIN TIMEOUT ", host );
                                               swarmAgentApi -> DoPingFailed(host);
                                               return;
                                          }
                                                               auto promised = chainNode -> RemoteGetHeaviestChain(count, cli);
                                                               if (promised.Wait())
                                                               {
                                                                   auto collection = promised.Get();
                                                                   if (collection.empty())
                                                                   {
                                                                       // must get at least genesis or this is an error case.
                                                                       swarmAgentApi -> DoPingFailed(host);
                                                                   }
                                                                   std::string blockId;

                                                                   std::string prevHash;
                                                                   for(auto &block : collection)
                                                                   {
                                                                       block.UpdateDigest();
                                                                       auto newblock = chainNode -> AddBlock(block);
                                                                       prevHash = block.prevString();
                                                                       if (newblock)
                                                                       {
                                                                           fetch::logger.Info("AGENT_API LOADCHAIN GOT ", block.summarise(),  (newblock?" NEW":" OLD") );
                                                                           swarmAgentApi -> DoNewBlockIdFound(host, block.hashString());
                                                                           if (block.loose())
                                                                           {
                                                                               fetch::logger.Info("AGENT_API LOADCHAIN LOOSE ", block.prevString() );
                                                                               pySwarm -> DoLooseBlock(host, block.prevString());
                                                                           }
                                                                       }
                                                                   }
                                                               }
                                                               else
                                                               {
                                                                   swarmAgentApi -> DoPingFailed(host);
                                                               }
                                                           }
                                                           catch (...)
                                                           {
                                                               swarmAgentApi -> DoPingFailed(host);
                                                           }
                                                       });
                                       });

    swarmAgentApi -> ToGetBlock([this, swarmAgentApi, swarmNode, chainNode, nnCore](const std::string &host, const std::string &blockid)
                                 {
                                   auto hashBytes = this -> blockIdToHash(blockid);
                                   auto pySwarm = this;
                                   swarmNode -> Post([swarmAgentApi, swarmNode, chainNode, nnCore, host, hashBytes, blockid, pySwarm]()
                                                     {
                                                         try
                                                         {
                                                             fetch::logger.Info("AGENT_API GET STARTS ", host, " ", blockid);
                                                             auto cli = nnCore -> ConnectTo(host);
                                                             if (!cli)
                                                             {
                                                                 fetch::logger.Info("AGENT_API GET TIMEOUT ", host, blockid );
                                                                 swarmAgentApi -> DoPingFailed(host);
                                                                 return;
                                                             }
                                                             auto promised = chainNode -> RemoteGetHeader(hashBytes, cli);
                                                             if (promised.Wait())
                                                             {
                                                                 auto found = promised.Get().first;
                                                                 auto block = promised.Get().second;
                                                                 if (found)
                                                                 {
                                                                     // add the block to the chainNode.
                                                                     block.UpdateDigest();
                                                                     auto newHash = block.hash();
                                                                     auto newBlockId = pySwarm -> hashToBlockId(newHash);

                                                                     auto newblock = chainNode -> AddBlock(block);

                                                                     auto block2 = block;
                                                                     chainNode -> Get(newHash, block2);

                                                                     fetch::logger.Info("AGENT_API GET NOW ", block.summarise());

                                                                     pySwarm -> DoBlockSupplied(host, block.hashString());
                                                                     if (newblock)
                                                                     {
                                                                         fetch::logger.Info("AGENT_API GET GOT ", block.summarise(),  (newblock?" NEW":" OLD") );
                                                                         swarmAgentApi -> DoNewBlockIdFound(host, block.hashString());
                                                                         if (block.loose())
                                                                         {
                                                                             fetch::logger.Info("AGENT_API GET LOOSE ", block.prevString() );
                                                                             pySwarm -> DoLooseBlock(host, block.prevString());
                                                                         }
                                                                         fetch::logger.Info("AGENT_API GET FORWARDING", block.summarise() );
                                                                         chainNode ->  Publish(fetch::ledger::MainChain::BLOCK_PUBLISH, block);
                                                                     }
                                                                 }
                                                                 else
                                                                 {
                                                                     fetch::logger.Info("AGENT_API GET DECLINED ", host, " ", blockid);
                                                                     pySwarm -> DoBlockNotSupplied(host, blockid);
                                                                 }
                                                             }
                                                             else
                                                             {
                                                                 fetch::logger.Info("AGENT_API GET WAIT TIMEOUT ", host, " ", blockid);
                                                                 pySwarm -> DoBlockNotSupplied(host, blockid);
                                                             }
                                                         }
                                                         catch (...)
                                                         {
                                                             swarmAgentApi -> DoPingFailed(host);
                                                         }
                                                     });
                                });
    swarmAgentApi -> ToGetKarma([swarmNode](const std::string &host)
                                {
                                  return swarmNode -> GetKarma(host);
                                });
    swarmAgentApi -> ToAddKarma([swarmNode](const std::string &host, double amount)
                                {
                                  swarmNode -> AddOrUpdate(host, amount);
                                });
    swarmAgentApi -> ToAddKarmaMax([swarmNode](const std::string &host, double amount, double limit)
                                   {
                                     if (swarmNode -> GetKarma(host) < limit)
                                       {
                                         swarmNode -> AddOrUpdate(host, amount);
                                       }
                                   });
    swarmAgentApi -> ToGetPeers([swarmAgentApi, swarmNode](uint32_t count, double minKarma)
                                {
                                  auto karmaPeers = swarmNode -> GetBestPeers(count, minKarma);
                                  std::list<std::string> results;
                                  for(auto &peer: karmaPeers)
                                    {
                                      results.push_back(peer.GetLocation().AsString());
                                    }
                                  if (results.empty())
                                    {
                                      swarmAgentApi -> DoPeerless();
                                    }
                                  return results;
                                });
    swarmAgentApi -> ToSetSitrep([swarmNode](const std::string &sitrep)
                                {
                                  return swarmNode -> SetSitrep(sitrep);
                                });
  }

  virtual ~PySwarm()
  {
    Stop();
  }

#define DELEGATE_CAPTURED ,this
#define DELEGATE_WRAPPER std::lock_guard<std::recursive_mutex> lock(this->mutex_);  pybind11::gil_scoped_acquire acquire;
#define DELEGATE swarmAgentApi_->


  /*-------------- MACHINE GENERATED CODE END -------------*/
virtual void OnIdle (pybind11::object func) { DELEGATE OnIdle (  [func DELEGATE_CAPTURED ](){  DELEGATE_WRAPPER func(); } ); }
virtual void OnPeerless (pybind11::object func) { DELEGATE OnPeerless (  [func DELEGATE_CAPTURED ](){  DELEGATE_WRAPPER func(); } ); }
virtual void DoPing (const std::string &host) {  DELEGATE DoPing ( host ); }
virtual void OnPingSucceeded (pybind11::object func) { DELEGATE OnPingSucceeded (  [func DELEGATE_CAPTURED ](const std::string &host){  DELEGATE_WRAPPER func(host); } ); }
virtual void OnPingFailed (pybind11::object func) { DELEGATE OnPingFailed (  [func DELEGATE_CAPTURED ](const std::string &host){  DELEGATE_WRAPPER func(host); } ); }
virtual void DoDiscoverPeers (const std::string &host, unsigned count) {  DELEGATE DoDiscoverPeers ( host,count ); }
virtual void OnNewPeerDiscovered (pybind11::object func) { DELEGATE OnNewPeerDiscovered (  [func DELEGATE_CAPTURED ](const std::string &host){  DELEGATE_WRAPPER func(host); } ); }
virtual void OnPeerDiscoverFail (pybind11::object func) { DELEGATE OnPeerDiscoverFail (  [func DELEGATE_CAPTURED ](const std::string &host){  DELEGATE_WRAPPER func(host); } ); }
virtual void DoBlockSolved (const std::string &blockdata) {  DELEGATE DoBlockSolved ( blockdata ); }
virtual void DoTransactionListBuilt (const std::list<std::string> &txnlist) {  DELEGATE DoTransactionListBuilt ( txnlist ); }
virtual void DoDiscoverBlocks (const std::string &host, uint32_t count) {  DELEGATE DoDiscoverBlocks ( host,count ); }
virtual void DoStopBlockDiscover (const std::string &host, uint32_t count) {  DELEGATE DoStopBlockDiscover ( host,count ); }
virtual void DoLoadChain (const std::string &host, uint32_t count) {  DELEGATE DoLoadChain ( host,count ); }
virtual void OnNewBlockIdFound (pybind11::object func) { DELEGATE OnNewBlockIdFound (  [func DELEGATE_CAPTURED ](const std::string &host, const std::string &blockid){  DELEGATE_WRAPPER func(host,blockid); } ); }
virtual void OnBlockIdRepeated (pybind11::object func) { DELEGATE OnBlockIdRepeated (  [func DELEGATE_CAPTURED ](const std::string &host, const std::string &blockid){  DELEGATE_WRAPPER func(host,blockid); } ); }
virtual void DoGetBlock (const std::string &host, const std::string &blockid) {  DELEGATE DoGetBlock ( host,blockid ); }
virtual void OnNewBlockAvailable (pybind11::object func) { DELEGATE OnNewBlockAvailable (  [func DELEGATE_CAPTURED ](const std::string &host, const std::string &blockid){  DELEGATE_WRAPPER func(host,blockid); } ); }
virtual std::string GetBlock (const std::string &blockid) { return DELEGATE GetBlock ( blockid ); }
virtual void VerifyBlock (const std::string &blockid, bool validity) {  DELEGATE VerifyBlock ( blockid,validity ); }
virtual void OnNewTxnListIdFound (pybind11::object func) { DELEGATE OnNewTxnListIdFound (  [func DELEGATE_CAPTURED ](const std::string &host, const std::string &txnlistid){  DELEGATE_WRAPPER func(host,txnlistid); } ); }
virtual void DoGetTxnList (const std::string &host, const std::string &txnlistid) {  DELEGATE DoGetTxnList ( host,txnlistid ); }
virtual void OnNewTxnListAvailable (pybind11::object func) { DELEGATE OnNewTxnListAvailable (  [func DELEGATE_CAPTURED ](const std::string &host, const std::string &txnlistid){  DELEGATE_WRAPPER func(host,txnlistid); } ); }
virtual std::string GetTxnList (const std::string &txnlistid) { return DELEGATE GetTxnList ( txnlistid ); }
virtual void AddKarma (const std::string &host, double karma) {  DELEGATE AddKarma ( host,karma ); }
virtual void SetSitrep (const std::string &sitrep) {  DELEGATE SetSitrep ( sitrep ); }
virtual void AddKarmaMax (const std::string &host, double karma, double limit) {  DELEGATE AddKarmaMax ( host,karma,limit ); }
virtual double GetKarma (const std::string &host) { return DELEGATE GetKarma ( host ); }
virtual double GetCost (const std::string &host) { return DELEGATE GetCost ( host ); }
virtual std::list<std::string> GetPeers (uint32_t count, double minKarma) { return DELEGATE GetPeers ( count,minKarma ); }
virtual std::string queryOwnLocation () { return DELEGATE queryOwnLocation (  ); }
    /*-------------- MACHINE GENERATED CODE END -------------*/

  std::function<void (const std::string &host, const std::string &blockid)> onNewRemoteHeaviestBlock_;
  virtual void DoNewRemoteHeaviestBlock(const std::string &host, const std::string &blockid) {
    worker_ -> Post([this, host, blockid]{
        if (onNewRemoteHeaviestBlock_) onNewRemoteHeaviestBlock_(host, blockid);
      });
  }
  virtual void pyOnNewRemoteHeaviestBlock(pybind11::object func) { OnNewRemoteHeaviestBlock( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnNewRemoteHeaviestBlock(std::function<void (const std::string &host, const std::string &blockid)> cb) { onNewRemoteHeaviestBlock_ = cb; }


  std::function<void (const std::string &host, const std::string &blockid)> onLooseBlock_;
  virtual void DoLooseBlock(const std::string &host, const std::string &blockid) {
    worker_ -> Post([this, host, blockid]{
        if (onLooseBlock_) onLooseBlock_(host, blockid);
      });
  }
  virtual void PyOnLooseBlock(pybind11::object func) { OnLooseBlock( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnLooseBlock(std::function<void (const std::string &host, const std::string &blockid)> cb) { onLooseBlock_ = cb; }


  std::function<void (const std::string &host, const std::string &blockid)> onBlockNotSupplied_;
  virtual void DoBlockNotSupplied(const std::string &host, const std::string &blockid) {
    worker_ -> Post([this, host, blockid]{
        if (onBlockNotSupplied_) onBlockNotSupplied_(host, blockid);
      });
  }
  virtual void PyOnBlockNotSupplied(pybind11::object func) { OnBlockNotSupplied( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnBlockNotSupplied(std::function<void (const std::string &host, const std::string &blockid)> cb) { onBlockNotSupplied_ = cb; }


  std::function<void (const std::string &host, const std::string &blockid)> onBlockSupplied_;
  virtual void DoBlockSupplied(const std::string &host, const std::string &blockid) {
    worker_ -> Post([this, host, blockid]{
        if (onBlockSupplied_) onBlockSupplied_(host, blockid);
      });
  }
  virtual void PyOnBlockSupplied(pybind11::object func) { OnBlockSupplied( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnBlockSupplied(std::function<void (const std::string &host, const std::string &blockid)> cb) { onBlockSupplied_ = cb; }



  std::string HeaviestBlock() const
  {
    return hashToBlockId(chainNode_ -> HeaviestBlock().hash() );
  }
};

}
}

#endif //__PY_SWARM_HPP
