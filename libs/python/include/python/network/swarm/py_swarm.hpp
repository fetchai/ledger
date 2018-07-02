#ifndef __PY_SWARM_HPP
#define __PY_SWARM_HPP

#include <iostream>
#include <string>
using std::cout;
using std::cerr;
using std::string;

#include <iostream>
#include "core/commandline/parameter_parser.hpp"

#include "network/swarm/swarm_service.hpp"
#include "network/swarm/swarm_node.hpp"

#include "network/parcels/swarm_parcel.hpp"
#include "network/parcels/swarm_parcel_node.hpp"
#include "network/parcels/swarm_parcel_protocol.hpp"
#include "network/swarm/swarm_service.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "network/parcels/swarm_agent_api_impl.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_http_interface.hpp"

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
    rnd_           = rhs.rnd_;
    swarmNode_     = rhs.swarmNode_;
    parcelNode_    = rhs.parcelNode_;
    swarmAgentApi_ = rhs.swarmAgentApi_;
  }
  PySwarm(PySwarm &&rhs)
  {
    nnCore_        = std::move(rhs.nnCore_);
    rnd_           = std::move(rhs.rnd_);
    swarmNode_     = std::move(rhs.swarmNode_);
    parcelNode_    = std::move(rhs.parcelNode_);
    swarmAgentApi_ = std::move(rhs.swarmAgentApi_);
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
    cout << "***** START"<<endl;
    swarmAgentApi_ -> Start();
    nnCore_ -> Start();
  }

  virtual void Stop()
  {
    lock_type lock(mutex_);
    nnCore_ -> Stop();
    swarmAgentApi_ -> Stop();
  }

  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;
  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;
  std::shared_ptr<fetch::swarm::SwarmNode> swarmNode_;
  std::shared_ptr<fetch::swarm::SwarmParcelNode> parcelNode_;
  std::shared_ptr<fetch::swarm::SwarmAgentApiImpl> swarmAgentApi_;
  std::shared_ptr<fetch::swarm::SwarmAgentNaive> agent_;
  std::shared_ptr<fetch::swarm::SwarmHttpInterface> httpInterface_;

  explicit PySwarm(unsigned int id, uint16_t rpcPort, uint16_t httpPort, unsigned int maxpeers, unsigned int idlespeed, unsigned int solvespeed)
  {
    std::cout << "PySwarm: rpc=" << rpcPort << " http=" << httpPort << std::endl;
    std::string identifier = "node-" + std::to_string(id);
    std::string myHost = "127.0.0.1:" + std::to_string(rpcPort);
    fetch::swarm::SwarmPeerLocation myHostLoc(myHost);

    auto nnCore = std::make_shared<fetch::network::NetworkNodeCore>(50, httpPort, rpcPort);
    auto rnd = std::make_shared<fetch::swarm::SwarmRandom>(id);
    auto swarmNode = std::make_shared<fetch::swarm::SwarmNode>(nnCore, identifier, maxpeers, rnd, myHost);
    auto parcelNode = std::make_shared<fetch::swarm::SwarmParcelNode>(nnCore);
    auto swarmAgentApi = std::make_shared<fetch::swarm::SwarmAgentApiImpl>(myHost, idlespeed);
    auto agent = std::make_shared<fetch::swarm::SwarmAgentNaive>(swarmAgentApi, identifier, id, rnd, maxpeers, solvespeed);

    auto httpInterface = std::make_shared<SwarmHttpInterface>(swarmNode);
    nnCore -> AddModule(*httpInterface);

    fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([](){ return time(0); });

    nnCore_ = nnCore;
    rnd_ = rnd;
    swarmNode_ = swarmNode;
    parcelNode_ = parcelNode;
    swarmAgentApi_ = swarmAgentApi;
    agent_ = agent;
    httpInterface_ = httpInterface;

    nnCore_ -> Start();

    // TODO(kll) Move this setup code somewhere more sensible.
    swarmAgentApi -> ToPing([swarmAgentApi, swarmNode](fetch::swarm::SwarmAgentApi &unused, const std::string &host)
                            {
                              std::cout << "ping this 1" << std::endl;
                              swarmNode -> Post([swarmAgentApi, swarmNode, host]()
                                           {
                                              std::cout << "ping this 2" << std::endl;
                                             try
                                               {
                                              std::cout << "ping this 3" << std::endl;
                                                 auto newPeer = swarmNode -> AskPeerForPeers(host);
                                              std::cout << "ping this 4" << std::endl;

                                                 if (newPeer.length()) {
                                              std::cout << "ping this 5" << std::endl;

                                                   if (!swarmNode -> IsOwnLocation(newPeer))
                                                     {
                                              std::cout << "ping this 6" << std::endl;
                                                       swarmAgentApi -> DoNewPeerDiscovered(newPeer);
                                                     }
                                                 }
                                                 swarmAgentApi -> DoPingSucceeded(host);
                                               }
                                             catch(fetch::serializers::SerializableException &x)
                                               {
                                              std::cout << "ping this 7" << std::endl;
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                             catch(fetch::swarm::SwarmException &x)
                                               {
                                              std::cout << "ping this 444" << std::endl;
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                             catch(std::invalid_argument &x)
                                               {
                                              std::cout << "ping this 8" << std::endl;
                                                 swarmAgentApi -> DoPingFailed(host);
                                               }
                                              std::cout << "ping this 9" << std::endl;
                                           });
                                              std::cout << "ping this 0" << std::endl;
                            });

     swarmAgentApi -> ToBlockSolved([swarmNode, parcelNode](const std::string &data)
                                    {
                                      parcelNode -> PublishParcel(std::make_shared<fetch::swarm::SwarmParcel>("block", data));
                                    });

     swarmAgentApi -> ToDiscoverBlocks([swarmAgentApi, swarmNode, parcelNode](const std::string &host, unsigned int count)
                                       {
                                         swarmNode ->Post([swarmAgentApi, swarmNode, parcelNode, host, count]()
                                                     {
                                                       try
                                                         {
                                                           std::cout << "ask peer for parcel ids" << std::endl;
                                                           auto blockids = parcelNode -> AskPeerForParcelIds(host, "block", count);
                                                           std::cout << "ask peer for parcel ids done" << std::endl;

                                                           for(auto &blockid : blockids)
                                                             {
                                                               if (!parcelNode -> HasParcel("block", blockid))
                                                                 {
                                                                   swarmAgentApi -> DoNewBlockIdFound(host, blockid);
                                                                 }
                                                               else
                                                                 {
                                                                   swarmAgentApi -> DoBlockIdRepeated(host, blockid);
                                                                 }
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
                                                       catch(std::invalid_argument &x)
                                                         {
                                                           swarmAgentApi -> DoPingFailed(host);
                                                         }
                                                     });
                                       });

     swarmAgentApi -> ToGetBlock([swarmAgentApi, swarmNode, parcelNode](const std::string &host, const std::string &blockid)
                                 {
                                   swarmNode -> Post([swarmAgentApi, swarmNode, parcelNode, host, blockid]()
                                                {
                                                  try
                                                    {
                                                      auto data = parcelNode -> AskPeerForParcelData(host, "block", blockid);

                                                      auto parcel = std::make_shared<fetch::swarm::SwarmParcel>("block", data);
                                                      if (parcel -> GetName() != blockid)
                                                        {
                                                          swarmAgentApi -> VerifyBlock(blockid, false);
                                                        }
                                                      else
                                                        {
                                                          if (!parcelNode -> HasParcel( "block", blockid))
                                                            {
                                                              parcelNode -> StoreParcel(parcel);
                                                              swarmAgentApi -> DoNewBlockAvailable(host, blockid);
                                                            }
                                                        }
                                                    }
                                                  catch(fetch::serializers::SerializableException &x)
                                                    {
                                                      cerr << " 5CAUGHT fetch::serializers::SerializableException" << x.what()<< endl;
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

     swarmAgentApi -> ToQueryBlock([swarmAgentApi, swarmNode, parcelNode] (const std::string &id)
                                   {
                                     if (!parcelNode -> HasParcel("block", id))
                                       {
                                         return std::string("<NO PARCEL>");
                                       }
                                     return parcelNode -> GetParcel("block", id) -> GetData();
                                   });

     swarmAgentApi -> ToVerifyBlock([swarmAgentApi, swarmNode, parcelNode](const std::string &id, bool validity)
                                    {
                                      if (parcelNode -> HasParcel("block", id))
                                        {
                                          if (validity)
                                            {
                                              parcelNode -> PublishParcel("block", id);
                                            }
                                          else
                                            {
                                              parcelNode -> DeleteParcel("block", id);
                                            }
                                        }
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

};

}
}

#endif //__PY_SWARM_HPP
