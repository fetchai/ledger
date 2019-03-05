
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

#include "network/muddle/network_id.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/muddle_endpoint.hpp"
//#include "ledger/dag/leader_board.future.hpp"
#include "ledger/dag/dag.hpp"
#include "ledger/protocols/dag_rpc_service.hpp"
#include "ledger/dag/dag_muddle_configuration.hpp"

#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "core/random/lfg.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/json/document.hpp"
#include "http/server.hpp"
#include "http/json_response.hpp"
#include "http/middleware/allow_origin.hpp"

#include "bootstrap_monitor.hpp"
#include "core/commandline/vt100.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "http/json_response.hpp"

#include <fstream>

#include <iostream>
#include <thread>
#include <chrono>
#include <random>

using fetch::network::NetworkManager;
using fetch::network::Peer;
using fetch::muddle::Muddle;
using NetworkId = fetch::muddle::NetworkId;
using fetch::muddle::rpc::Client;
using fetch::service::Promise;
using fetch::commandline::ParamsParser;
using fetch::ledger::DAGNode;
using fetch::ledger::DAGRpcService;
//using fetch::ledger::LeaderBoard;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::crypto::ECDSASigner;
using fetch::byte_array::ToHex;
using fetch::http::HTTPServer;
using fetch::http::Method;
using fetch::http::ViewParameters;
using fetch::http::HTTPRequest;
using fetch::http::HTTPResponse;
using fetch::http::CreateJsonResponse;

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

using Rng = std::mt19937_64;
using RngWord = Rng::result_type;

using HttpServerPtr = std::unique_ptr<HTTPServer>;
using DAG = fetch::ledger::DAG;

/**
 * Tests to write:
 *  - DAG sync test starting at same time
 *  - DAG sync test starting with long delay
 *  - Multi node system where nodes come and go.
 *
 * Next step:
 *  - Clean DAG code up
 *  - Make entry in block for DAG hashes
 *  - Implement following into constellation: 
 *
 * Auction events
 * ──────────────                       │                   │
 *  ┌───┐   ┌────────────────────────GetSegment ┌─────────┐ │
 *  │   ◀──▶│          DAG           │◀─┼───────▶         │ │
 *  │ M │   └────────────────────────┘  │       │ Extract │ │
 *  │ u │                               │       │ Segment │ │
 *  │ d │   ┌────────────────────────OnBlock    │         OnAuction
 *  │ d ◀──▶│       Blockchain       ├──┼─────┬─▶         ├─┼─────▶
 *  │ l │   └────────────────────────┴──┼────┐│ └─────────┘ │
 *  │ e │                               │    ││             OnBlock
 *  │   │   ┌────────────────────────┐  │    │└─────────────┼─────▶
 *  │   ◀──▶│     Random Beacon      │  │    │             OnRevert
 *  └───┘   └────────────────────────┘  │    └──────────────┼─────▶
 *                                      │                   │
 * Private submodules                   │Internal           │Public
 *
 * Illustration of internal architecture for proof-of-useful-work
 * consensus.
 *
 **/

/**
 * How to run:
 *
 * ./libs/ledger/examples/example-ledger-dag-consensus ../libs/ledger/examples/dag_consensus/configurations/node1.json
 * ./libs/ledger/examples/example-ledger-dag-consensus ../libs/ledger/examples/dag_consensus/configurations/node2.json
 **/

static DAGNode GenerateNode(ConstByteArray const &data, Rng &rng, ECDSASigner &certificate, DAG &dag)
{
  // build up the DAG node
  DAGNode node;
  node.contents = data;
  node.identity = certificate.identity();

  // TODO: Set previous and type
  auto prev_candidates = dag.last_nodes();

  if(prev_candidates.size() > 0 )
  {
    node.previous.push_back( prev_candidates[ rng() % prev_candidates.size() ]);
  }

  if(prev_candidates.size() > 3 )
  {
    node.previous.push_back( prev_candidates[ rng() % prev_candidates.size() ]);
  }

  node.Finalise();

  if(!certificate.Sign(node.hash))
  {
    throw std::runtime_error("Signing failed");
  }

  node.signature = certificate.signature();

#if 1
  // DEBUG: Verification
  {
    fetch::crypto::ECDSAVerifier verifier(node.identity);
    if (!verifier.Verify(node.hash, node.signature))
    {
      throw std::runtime_error("Verification failed");
    }
  }
#endif

  return node;
}

static DAGNode GenerateNode(Rng &rng, ECDSASigner &certificate, DAG &dag)
{
  static constexpr std::size_t BUFFER_LEN = 2048;
  static constexpr std::size_t BUFFER_WORD_LEN = BUFFER_LEN / sizeof(RngWord);
  static_assert((BUFFER_LEN % sizeof(RngWord)) == 0, "");

  // generate random contents for the DAG node
  ByteArray buffer;
  buffer.Resize(BUFFER_LEN);
  auto *buffer_raw = reinterpret_cast<RngWord *>(buffer.pointer());
  for (std::size_t i = 0; i < BUFFER_WORD_LEN; ++i)
  {
    buffer_raw[i] = rng();
  }

  return GenerateNode(buffer, rng, certificate, dag);
}

int main(int argc, char **argv)
{
  std::unique_ptr<fetch::crypto::ECDSASigner> signer = std::make_unique<fetch::crypto::ECDSASigner>();
  std::unique_ptr<fetch::crypto::ECDSASigner> certificate = std::make_unique<fetch::crypto::ECDSASigner>();  

  if(argc <= 1)
  {

    std::cout << "New credentials" << std::endl;
    signer->GenerateKeys();
    std::cout << "Parameters: " << signer->identity().parameters() << std::endl;
    std::cout << "Public key: " << fetch::byte_array::ToBase64(signer->identity().identifier()) << std::endl;    
    std::cout << "Private key: " << fetch::byte_array::ToBase64(signer->private_key()) << std::endl;

    return 0;
  }  

  ParamsParser params;
  params.Parse(argc, argv);

  // Loading settings
  fetch::json::JSONDocument doc;

  std::ifstream t(params.GetArg(1));
  std::string config_txt((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  doc.Parse(config_txt);

  if(doc.Has("public_key"))
  {
    std::cout << "Setting key from config" << std::endl;
    if(!doc.Has("private_key"))
    {
      std::cerr << "please specify private key or delete public key" << std::endl;
      exit(-1);
    }

    fetch::byte_array::ByteArray public_key = doc["public_key"].As<fetch::byte_array::ConstByteArray>();
    fetch::byte_array::ByteArray private_key = doc["private_key"].As<fetch::byte_array::ConstByteArray>();
    signer->Load(fetch::byte_array::FromBase64(private_key)); 
    certificate->Load(fetch::byte_array::FromBase64(private_key));  
  }
  else
  {
    signer->GenerateKeys();
    certificate->Load( signer->private_key().Copy() );
  }

  // Setting  up
  fetch::network::DumpNetworkActivityTo("netdump-"+ params.GetArg(2));
  std::cout << "XXX: " << fetch::network::MonitoringClass::monitor.get() <<std::endl;
  DUMP_INCOMING_MESSAGE("xxx","yyy", "zzz");
//  exit(0);

  NetworkManager network_manager("nid-name", params.GetParam<std::size_t>("threads", 16) );
  network_manager.Start();
  std::cout << "Node certificiate: " << fetch::byte_array::ToBase64(signer->identity().identifier()) << std::endl;

  uint16_t port = params.GetParam<uint16_t>("port",0);
  if(port == 0)
  {
    port = doc["port"].As<uint16_t>();
  }

  // Starting muddle
  std::cout << "Listening on " << port << std::endl;

  NetworkId nid("dag-testnet");
  Muddle muddle{nid,  std::move(signer), network_manager};

  std::cout << "Creating list of peers" << std::endl;
  std::vector< fetch::network::Uri > connect_to;
  auto peers = doc["peers"];
  for(std::size_t i=0; i < peers.size(); ++i)
  {
    std::cout << i <<") " << peers[i].As<fetch::byte_array::ConstByteArray>() << std::endl;
    connect_to.push_back( fetch::network::Uri(std::string(peers[i].As<fetch::byte_array::ConstByteArray>()))  );
  }

  std::cout << "Starting muddle" << std::endl;
  muddle.Start({port}, connect_to);

  // TODO(EJF): Quick fix, needs longer term improvement
  sleep_for(milliseconds{3000});

  // Building initial stake holder board
  /*
  std::vector<fetch::ledger::Candidacy> stake_holders;
  std::cout << "Setting committee up" << std::endl;
  auto committee = doc["committee"];
  for(std::size_t i=0; i < committee.size(); ++i)
  {    
    auto param = committee[i]["parameters"].As<fetch::byte_array::ConstByteArray>();
    auto pk = committee[i]["key"].As<fetch::byte_array::ConstByteArray>();

    fetch::crypto::Identity identity{ param, fetch::byte_array::FromBase64(pk) };
    stake_holders.push_back( {identity, 5} );
  }
  */

  // Initial genesis
  std::vector< uint64_t > genesis_entropy = {128831, 86942827};  

  // Creating the consensus controller
  std::fstream outfile(params.GetArg(2), std::ios::out);
  fetch::ledger::DAG dag;

  std::atomic< int > node_count{0};
  fetch::mutex::Mutex file_mutex{__LINE__, __FILE__};
   dag.OnNewNode([&outfile, &node_count, &file_mutex](DAGNode n)
  {
    FETCH_LOCK(file_mutex);
    using Clock     = std::chrono::system_clock;
    using Timepoint = Clock::time_point;    
    Timepoint const now           = Clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    outfile <<  ToHex(n.hash) << "  --  " <<  std::put_time(std::localtime(&now_c), "%F %T") << std::endl;
    ++node_count;
    std::cout << "Node Count: " << node_count  << std::endl;
  });

  DAGRpcService controller(muddle, muddle.AsEndpoint(), dag);
  controller.SetCertificate( certificate->private_key() );

  std::random_device rd;
  Rng rng(rd());

  std::size_t nodes_generated = 0;
  std::string line;
  FETCH_UNUSED(line);

  controller.Synchronise();

  std::size_t peer_count = muddle.NumPeers();
  for (;;)
  {
    std::cout << "Hit enter" << std::endl;
    std::string dummy;
//    std::getline(std::cin, dummy);
    peer_count = std::max(peer_count, muddle.NumPeers());
    if(muddle.NumPeers() < peer_count)
    {
      std::cout << "STOPPING!" << std::endl;
      exit(-1);
    }

    DAGNode node = GenerateNode(rng, *certificate, dag);

    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // send the node around the network

    controller.BroadcastDAGNode(node);
    dag.Push(node);
        
    ++nodes_generated;
  }

  return 0;
}

