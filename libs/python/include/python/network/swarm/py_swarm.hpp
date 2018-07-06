#ifndef __PY_SWARM_HPP
#define __PY_SWARM_HPP

#include <iostream>
#include <string>
using std::cout;
using std::cerr;
using std::string;

#include <iostream>
#include "core/commandline/parameter_parser.hpp"
#include "network/swarm/swarm_node.hpp"

#include "network/generics/network_node_core.hpp"
#include "network/parcels/swarm_parcel.hpp"
#include "network/parcels/swarm_parcel_node.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "network/parcels/swarm_agent_api_impl.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "python/worker/python_worker.hpp"
#include "ledger/main_chain_node.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/byte_array/decoders.hpp"

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
  PySwarm(const PySwarm &rhs)
  {
    nnCore_        = rhs.nnCore_;
    parcelNode_    = rhs.parcelNode_;
    rnd_           = rhs.rnd_;
    swarmAgentApi_ = rhs.swarmAgentApi_;
    swarmNode_     = rhs.swarmNode_;
    worker_        = rhs.worker_;
    chainNode_     = rhs.chainNode_;
  }
  PySwarm(PySwarm &&rhs)
  {
    nnCore_        = std::move(rhs.nnCore_);
    parcelNode_    = std::move(rhs.parcelNode_);
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
  std::shared_ptr<fetch::swarm::SwarmParcelNode> parcelNode_;
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

  explicit PySwarm(unsigned int id, uint16_t rpcPort, uint16_t httpPort, unsigned int maxpeers, unsigned int idlespeed)
  {
    std::string identifier = "node-" + std::to_string(id);
    std::string myHost = "127.0.0.1:" + std::to_string(rpcPort);
    fetch::swarm::SwarmPeerLocation myHostLoc(myHost);

    auto worker = PythonWorker::instance();
    auto nnCore = std::make_shared<fetch::network::NetworkNodeCore>(20, httpPort, rpcPort);
    auto rnd = std::make_shared<fetch::swarm::SwarmRandom>(id);
    auto swarmNode = std::make_shared<fetch::swarm::SwarmNode>(nnCore, identifier, maxpeers, rnd, myHost);

    auto httpModule = std::make_shared<SwarmHttpModule>(swarmNode);
    auto parcelNode = std::make_shared<fetch::swarm::SwarmParcelNode>(nnCore);
    auto chainNode = std::make_shared<fetch::ledger::MainChainNode>(nnCore, id);
    auto swarmAgentApi = std::make_shared<fetch::swarm::SwarmAgentApiImpl<PythonWorker>>(worker, myHost, idlespeed);
    worker -> UseCore(nnCore);

    auto chain = std::make_shared<fetch::ledger::MainChain>();

    nnCore -> AddModule(httpModule);
    fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([](){ return time(0); });

    httpModule_ = httpModule;
    nnCore_ = nnCore;
    parcelNode_ = parcelNode;
    rnd_ = rnd;
    swarmAgentApi_ = swarmAgentApi;
    swarmNode_ = swarmNode;
    worker_ = worker;
    chainNode_ = chainNode;

    nnCore_ -> Start();

    // TODO(kll) Move this setup code somewhere more sensible.
    swarmAgentApi -> ToPing([swarmAgentApi, swarmNode](fetch::swarm::SwarmAgentApi &unused, const std::string &host)
                            {
                              swarmNode -> Post([swarmAgentApi, swarmNode, host]()
                                           {
                                             try
                                               {
                                                 auto newPeer = swarmNode -> AskPeerForPeers(host);
                                                 if (newPeer.length()) {
                                                   std::cout << "~~ GOT PEER" << std::endl;
                                                   if (!swarmNode -> IsOwnLocation(newPeer))
                                                     {
                                                       std::cout << "~~ GOT REMOTE PEER" << std::endl;
                                                       if (!swarmNode -> IsExistingPeer(newPeer))
                                                         {
                                                           std::cout << "~~ GOT NEW REMOTE PEER" << std::endl;
                                                           swarmNode -> AddOrUpdate(host, 0);
                                                           swarmAgentApi -> DoNewPeerDiscovered(newPeer);
                                                         }
                                                     }
                                                 }
                                                 swarmAgentApi -> DoPingSucceeded(host);
                                               }
                                             catch(fetch::serializers::SerializableException &x)
                                               {
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                             catch(fetch::swarm::SwarmException &x)
                                               {
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                             catch(network::NetworkNodeCoreBaseException &x)
                                               {
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                             catch(std::invalid_argument &x)
                                               {
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                           });
                            });

    swarmAgentApi -> ToDiscoverBlocks([swarmAgentApi, swarmNode, chainNode, nnCore](const std::string &host, unsigned int count)
                                       {
                                         swarmNode ->Post([swarmAgentApi, nnCore, chainNode, host, count]()
                                                     {
                                                       try
                                                         {
                                                           auto promised = chainNode -> RemoteGetHeaviestChain(
                                                                                                               count,
                                                                                                               nnCore -> ConnectTo(host)
                                                                                                               );
                                                           if (promised.Wait())
                                                             {
                                                               auto collection = promised.Get();
                                                               for(auto hash : collection)
                                                                 {
                                                                   // add block
                                                                   // report unattached blocks here.
                                                                 }
                                                             }
                                                           else
                                                             {
                                                               swarmAgentApi -> DoPingFailed(host);
                                                             }
                                                         }
                                                       catch(fetch::serializers::SerializableException &x)
                                                         {
                                                           cerr << " 3CAUGHT fetch::serializers::SerializableException" << x.what()<< endl;
                                                           swarmAgentApi -> DoPingFailed(host);
                                                         }
                                                       catch(fetch::swarm::SwarmException &x)
                                                         {
                                                           cerr << " 4CAUGHT SwarmException" << x.what()<< endl;
                                                           swarmAgentApi -> DoPingFailed(host);
                                                         }
                                                       catch(network::NetworkNodeCoreBaseException &x)
                                                         {
                                                           std::cout << x.what() << std::endl;
                                                           swarmAgentApi -> DoPingFailed(host);
                                                         }
                                                       catch(std::invalid_argument &x)
                                                         {
                                                           swarmAgentApi -> DoPingFailed(host);
                                                         }
                                                     });
                                       });

    swarmAgentApi -> ToGetBlock([this, swarmAgentApi, swarmNode, chainNode, nnCore](const std::string &host, const std::string &blockid)
                                 {
                                   auto hash = this -> blockIdToHash(blockid);
                                   auto pySwarm = this;
                                   swarmNode -> Post([swarmAgentApi, swarmNode, chainNode, nnCore, host, hash, blockid, pySwarm]()
                                                {
                                                  try
                                                    {
                                                      auto promised = chainNode -> RemoteGetHeader(
                                                                                                   hash,
                                                                                                   nnCore -> ConnectTo(host)
                                                                                                   );
                                                      if (promised.Wait())
                                                        {
                                                          auto found = promised.Get().first;
                                                          auto newHash = promised.Get().second.hash();
                                                          if (found)
                                                            {
                                                              // add the block to the chainNode.
                                                              auto newBlockId = pySwarm -> hashToBlockId(newHash);
                                                              pySwarm -> DoBlockSupplied(host, blockid);
                                                            }
                                                          else
                                                            {
                                                              pySwarm -> DoBlockNotSupplied(host, blockid);
                                                            }
                                                        }
                                                      else
                                                        {
                                                              pySwarm -> DoBlockNotSupplied(host, blockid);
                                                        }
                                                    }
                                                  catch(fetch::serializers::SerializableException &x)
                                                    {
                                                      cerr << " 5CAUGHT fetch::serializers::SerializableException" << x.what()<< endl;
                                                      swarmAgentApi -> DoPingFailed(host);
                                                    }
                                                  catch(network::NetworkNodeCoreBaseException &x)
                                                    {
                                                      std::cout << x.what() << std::endl;
                                                      swarmAgentApi -> DoPingFailed(host);
                                                    }
                                                  catch(fetch::swarm::SwarmException &x)
                                                    {
                                                      cerr << " 6CAUGHT SwarmException" << x.what()<< endl;
                                                      swarmAgentApi -> DoPingFailed(host);
                                                    }
                                                  catch(std::invalid_argument &x)
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
    swarmAgentApi -> ToGetPeers([swarmAgentApi, swarmNode](unsigned int count, double minKarma)
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

     std::cout << "PySwarm: BUILT" << std::endl;
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
virtual void DoDiscoverBlocks (const std::string &host, unsigned int count) {  DELEGATE DoDiscoverBlocks ( host,count ); }
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
virtual void AddKarmaMax (const std::string &host, double karma, double limit) {  DELEGATE AddKarmaMax ( host,karma,limit ); }
virtual double GetKarma (const std::string &host) { return DELEGATE GetKarma ( host ); }
virtual double GetCost (const std::string &host) { return DELEGATE GetCost ( host ); }
virtual std::list<std::string> GetPeers (unsigned int count, double minKarma) { return DELEGATE GetPeers ( count,minKarma ); }
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
  virtual void OnLooseBlock(pybind11::object func) { OnLooseBlock( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnLooseBlock(std::function<void (const std::string &host, const std::string &blockid)> cb) { onLooseBlock_ = cb; }


  std::function<void (const std::string &host, const std::string &blockid)> onBlockNotSupplied_;
  virtual void DoBlockNotSupplied(const std::string &host, const std::string &blockid) {
    worker_ -> Post([this, host, blockid]{
        if (onBlockNotSupplied_) onBlockNotSupplied_(host, blockid);
      });
  }
  virtual void OnBlockNotSupplied(pybind11::object func) { OnBlockNotSupplied( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnBlockNotSupplied(std::function<void (const std::string &host, const std::string &blockid)> cb) { onBlockNotSupplied_ = cb; }


  std::function<void (const std::string &host, const std::string &blockid)> onBlockSupplied_;
  virtual void DoBlockSupplied(const std::string &host, const std::string &blockid) {
    worker_ -> Post([this, host, blockid]{
        if (onBlockSupplied_) onBlockSupplied_(host, blockid);
      });
  }
  virtual void OnBlockSupplied(pybind11::object func) { OnBlockSupplied( [func,this](const std::string &host, const std::string &blockid){ DELEGATE_WRAPPER func(host, blockid); } ); }
  virtual void OnBlockSupplied(std::function<void (const std::string &host, const std::string &blockid)> cb) { onBlockSupplied_ = cb; }



  std::string HeaviestBlock() const
  {
    return hashToBlockId(chainNode_ -> HeaviestBlock().hash() );
  }
};

}
}

#endif //__PY_SWARM_HPP
