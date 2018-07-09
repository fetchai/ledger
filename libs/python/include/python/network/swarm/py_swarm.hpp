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
    tm_            = rhs.tm_;
    rnd_           = rhs.rnd_;
    node_          = rhs.node_;
    service_       = rhs.service_;
    swarmAgentApi_ = rhs.swarmAgentApi_;
  }
  PySwarm(PySwarm &&rhs)
  {
    tm_            = std::move(rhs.tm_);
    rnd_           = std::move(rhs.rnd_);
    node_          = std::move(rhs.node_);
    service_       = std::move(rhs.service_);
    swarmAgentApi_ = std::move(rhs.swarmAgentApi_);
  }
  PySwarm operator=(const PySwarm &rhs)  = delete;
  PySwarm operator=(PySwarm &&rhs) = delete;
  bool operator==(const PySwarm &rhs) const = delete;
  bool operator<(const PySwarm &rhs) const = delete;

  std::shared_ptr<fetch::swarm::SwarmRandom> rnd_;
  std::shared_ptr<fetch::swarm::SwarmNode> node_;
  std::shared_ptr<fetch::swarm::SwarmService> service_;
  std::shared_ptr<fetch::network::NetworkManager> tm_;
  std::shared_ptr<fetch::swarm::SwarmAgentApiImpl> swarmAgentApi_;


  std::shared_ptr<fetch::swarm::SwarmParcelNode> parcelNode_;
  std::shared_ptr<fetch::swarm::SwarmParcelProtocol> parcelProtocol_;

  typedef std::recursive_mutex MUTEX_T;
  typedef std::lock_guard<std::recursive_mutex> LOCK_T;
  MUTEX_T mutex_;

  virtual void Start()
  {
    LOCK_T lock(mutex_);
    cout << "***** START"<<endl;
    swarmAgentApi_ -> Start();
    tm_ -> Start();
  }

  virtual void Stop()
  {
    LOCK_T lock(mutex_);
    tm_ -> Stop();
    swarmAgentApi_ -> Stop();
  }

  explicit PySwarm(unsigned int id, uint16_t portNumber, unsigned int maxpeers, unsigned int idlespeed, unsigned int solvespeed)
  {
    fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([](){ return time(0); });
    std::string identifier = "node-" + std::to_string(id);
    std::string myHost = "127.0.0.1:" + std::to_string(portNumber);
    fetch::swarm::SwarmPeerLocation myHostLoc(myHost);


    auto tm = std::make_shared<fetch::network::NetworkManager>(2);
    auto rnd = std::make_shared<fetch::swarm::SwarmRandom>(id);
    auto node = std::make_shared<fetch::swarm::SwarmNode>(*tm, identifier, maxpeers, rnd, myHost, fetch::protocols::FetchProtocols::SWARM);
    auto service = std::make_shared<fetch::swarm::SwarmService>(*tm, portNumber, node, myHost, idlespeed);
    auto swarmAgentApi = std::make_shared<fetch::swarm::SwarmAgentApiImpl>(myHost, idlespeed);

    auto parcelNode = std::make_shared<fetch::swarm::SwarmParcelNode>(node, fetch::protocols::FetchProtocols::PARCEL);
    auto parcelProtocol = std::make_shared<fetch::swarm::SwarmParcelProtocol>(parcelNode);

    service -> addRpcProtocol(fetch::protocols::FetchProtocols::PARCEL, parcelProtocol);

    tm_ = tm;
    rnd_ = rnd;
    node_ = node;
    service_ = service;
    swarmAgentApi_ = swarmAgentApi;

    parcelNode_ = parcelNode;
    parcelProtocol_ = parcelProtocol;

    // TODO(kll) Move this setup code somewhere more sensible.
    swarmAgentApi -> ToPing([swarmAgentApi, node](fetch::swarm::SwarmAgentApi &unused, const std::string &host)
                            {
                              node -> Post([swarmAgentApi, node, host]()
                                           {
                                             try
                                               {
                                                 auto newPeer = node -> AskPeerForPeers(host);

                                                 if (newPeer.length()) {

                                                   if (!node -> IsOwnLocation(newPeer))
                                                     {
                                                       swarmAgentApi -> DoNewPeerDiscovered(newPeer);
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
                                           });
                            });

     swarmAgentApi -> ToBlockSolved([node, parcelNode](const std::string &data)
                                    {
                                      parcelNode -> PublishParcel(std::make_shared<fetch::swarm::SwarmParcel>("block", data));
                                    });

     swarmAgentApi -> ToDiscoverBlocks([swarmAgentApi, node, parcelNode](const std::string &host, unsigned int count)
                                       {
                                         node ->Post([swarmAgentApi, node, parcelNode, host, count]()
                                                     {
                                                       try
                                                         {
                                                           auto blockids = parcelNode -> AskPeerForParcelIds(host, "block", count);

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
                                                     });
                                       });

     swarmAgentApi -> ToGetBlock([swarmAgentApi, node, parcelNode](const std::string &host, const std::string &blockid)
                                 {
                                   node -> Post([swarmAgentApi, node, parcelNode, host, blockid]()
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
                                                });
                                 });
    swarmAgentApi -> ToGetKarma([node](const std::string &host)
                                {
                                  return node -> GetKarma(host);
                                });
    swarmAgentApi -> ToAddKarma([node](const std::string &host, double amount)
                                {
                                  node -> AddOrUpdate(host, amount);
                                });
    swarmAgentApi -> ToAddKarmaMax([node](const std::string &host, double amount, double limit)
                                   {
                                     if (node -> GetKarma(host) < limit)
                                       {
                                         node -> AddOrUpdate(host, amount);
                                       }
                                   });
    swarmAgentApi -> ToGetPeers([swarmAgentApi, node](unsigned int count, double minKarma)
                                {
                                  auto karmaPeers = node -> GetBestPeers(count, minKarma);
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

     swarmAgentApi -> ToQueryBlock([swarmAgentApi, node, parcelNode] (const std::string &id)
                                   {
                                     if (!parcelNode -> HasParcel("block", id))
                                       {
                                         return std::string("<NO PARCEL>");
                                       }
                                     return parcelNode -> GetParcel("block", id) -> GetData();
                                   });

     swarmAgentApi -> ToVerifyBlock([swarmAgentApi, node, parcelNode](const std::string &id, bool validity)
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
