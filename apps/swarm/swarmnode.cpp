#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>

#include "core/commandline/parameter_parser.hpp"
#include "network/details/thread_manager.hpp"
#include "network/parcels/swarm_agent_api_impl.hpp"
#include "network/parcels/swarm_agent_naive.hpp"
#include "network/parcels/swarm_parcel_node.hpp"
#include "network/parcels/swarm_parcel_protocol.hpp"
#include "network/service/protocol.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_node.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "network/swarm/swarm_service.hpp"

using uint = unsigned int;

int main(int argc, const char *argv[])
{
  fetch::commandline::ParamsParser params;
  params.Parse(argc, argv);

  uint16_t     portNumber = params.GetParam<uint16_t>("port", 0);
  unsigned int maxpeers   = params.GetParam<unsigned int>("maxpeers", 0);
  unsigned int idlespeed  = params.GetParam<unsigned int>("idlespeed", 1000);
  unsigned int solvespeed = params.GetParam<unsigned int>("solvespeed", 1000);
  std::list<fetch::swarm::SwarmPeerLocation> peers =
      fetch::swarm::SwarmPeerLocation::ParsePeerListString(
          params.GetParam<std::string>("peers", ""));

  fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([]() { return time(0); });

  fetch::network::ThreadManager tm(30);

  unsigned int id = params.GetParam<unsigned int>("id", 0);

  std::string identifier = "node-" + std::to_string(id);

  std::string                     myHost = "127.0.0.1:" + std::to_string(portNumber);
  fetch::swarm::SwarmPeerLocation myHostLoc(myHost);

  auto rnd = std::make_shared<fetch::swarm::SwarmRandom>(id);

  std::shared_ptr<fetch::swarm::SwarmNode> node = std::make_shared<fetch::swarm::SwarmNode>(
      tm, identifier, maxpeers, rnd, myHost, fetch::protocols::FetchProtocols::SWARM);

  auto service =
      std::make_shared<fetch::swarm::SwarmService>(tm, portNumber, node, myHost, idlespeed);
  auto swarmAgentApi = std::make_shared<fetch::swarm::SwarmAgentApiImpl>(myHost, idlespeed);
  auto agent = std::make_shared<fetch::swarm::SwarmAgentNaive>(swarmAgentApi, identifier, id, rnd,
                                                               maxpeers, solvespeed);

  auto parcelNode = std::make_shared<fetch::swarm::SwarmParcelNode>(
      node, fetch::protocols::FetchProtocols::PARCEL);

  auto parcelProtocol = std::make_shared<fetch::swarm::SwarmParcelProtocol>(parcelNode);

  service->addRpcProtocol(fetch::protocols::FetchProtocols::PARCEL, parcelProtocol);

  // TODO(katie) move the handling of the node functions off the ThreadManager
  // running the io_service.

  swarmAgentApi->ToPing([swarmAgentApi, node, parcelNode](fetch::swarm::SwarmAgentApi &unused,
                                                          const std::string &          host) {
    node->Post([swarmAgentApi, node, parcelNode, host]() {
      try
      {
        auto newPeer = node->AskPeerForPeers(host);

        if (newPeer.length())
        {

          if (!node->IsOwnLocation(newPeer))
          {
            swarmAgentApi->DoNewPeerDiscovered(newPeer);
          }
        }
        swarmAgentApi->DoPingSucceeded(host);
      }
      catch (fetch::serializers::SerializableException &x)
      {
        cerr << " 1CAUGHT fetch::serializers::SerializableException:" << x.what() << endl;
        swarmAgentApi->DoPingFailed(host);
      }
      catch (fetch::swarm::SwarmException &x)
      {
        cerr << " 2CAUGHT SwarmException" << x.what() << endl;
        swarmAgentApi->DoPingFailed(host);
      }
    });
  });

  swarmAgentApi->ToBlockSolved([node, parcelNode](const std::string &data) {
    parcelNode->PublishParcel(std::make_shared<fetch::swarm::SwarmParcel>("block", data));
  });

  swarmAgentApi->ToDiscoverBlocks(
      [swarmAgentApi, node, parcelNode](const std::string &host, unsigned int count) {
        node->Post([swarmAgentApi, node, parcelNode, host, count]() {
          try
          {
            auto blockids = parcelNode->AskPeerForParcelIds(host, "block", count);

            for (auto &blockid : blockids)
            {
              if (!parcelNode->HasParcel("block", blockid))
              {
                swarmAgentApi->DoNewBlockIdFound(host, blockid);
              }
              else
              {
                swarmAgentApi->DoBlockIdRepeated(host, blockid);
              }
            }
          }
          catch (fetch::serializers::SerializableException &x)
          {
            cerr << " 3CAUGHT fetch::serializers::SerializableException" << x.what() << endl;
            swarmAgentApi->DoPingFailed(host);
          }
          catch (fetch::swarm::SwarmException &x)
          {
            cerr << " 4CAUGHT SwarmException" << x.what() << endl;
            swarmAgentApi->DoPingFailed(host);
          }
        });
      });

  swarmAgentApi->ToGetBlock(
      [swarmAgentApi, node, parcelNode](const std::string &host, const std::string &blockid) {
        node->Post([swarmAgentApi, node, parcelNode, host, blockid]() {
          try
          {
            auto data = parcelNode->AskPeerForParcelData(host, "block", blockid);

            auto parcel = std::make_shared<fetch::swarm::SwarmParcel>("block", data);
            if (parcel->GetName() != blockid)
            {
              swarmAgentApi->VerifyBlock(blockid, false);
            }
            else
            {
              if (!parcelNode->HasParcel("block", blockid))
              {
                parcelNode->StoreParcel(parcel);
                swarmAgentApi->DoNewBlockAvailable(host, blockid);
              }
            }
          }
          catch (fetch::serializers::SerializableException &x)
          {
            cerr << " 5CAUGHT fetch::serializers::SerializableException" << x.what() << endl;
            swarmAgentApi->DoPingFailed(host);
          }
          catch (fetch::swarm::SwarmException &x)
          {
            cerr << " 6CAUGHT SwarmException" << x.what() << endl;
            swarmAgentApi->DoPingFailed(host);
          }
        });
      });
  swarmAgentApi->ToGetKarma([node](const std::string &host) { return node->GetKarma(host); });
  swarmAgentApi->ToAddKarma(
      [node](const std::string &host, double amount) { node->AddOrUpdate(host, amount); });
  swarmAgentApi->ToAddKarmaMax([node](const std::string &host, double amount, double limit) {
    if (node->GetKarma(host) < limit)
    {
      node->AddOrUpdate(host, amount);
    }
  });
  swarmAgentApi->ToGetPeers([swarmAgentApi, node, parcelNode](unsigned int count, double minKarma) {
    auto                   karmaPeers = node->GetBestPeers(count, minKarma);
    std::list<std::string> results;
    for (auto &peer : karmaPeers)
    {
      results.push_back(peer.GetLocation().AsString());
    }
    if (results.empty())
    {
      swarmAgentApi->DoPeerless();
    }
    return results;
  });

  swarmAgentApi->ToQueryBlock([swarmAgentApi, node, parcelNode](const std::string &id) {
    if (!parcelNode->HasParcel("block", id))
    {
      return std::string("<NO PARCEL>");
    }
    return parcelNode->GetParcel("block", id)->GetData();
  });

  swarmAgentApi->ToVerifyBlock(
      [swarmAgentApi, node, parcelNode](const std::string &id, bool validity) {
        if (parcelNode->HasParcel("block", id))
        {
          if (validity)
          {
            parcelNode->PublishParcel("block", id);
          }
          else
          {
            parcelNode->DeleteParcel("block", id);
          }
        }
      });

  swarmAgentApi->Start();

  for (auto &peer : peers)
  {
    agent->addInitialPeer(peer.AsString());
  }

  tm.Start();

  int dummy;

  std::cout << "press any key to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();
}
