#include <iostream>
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

#include "ledger/upow_consensus/leader_tracker.hpp"
#include "ledger/upow_consensus/dag.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "core/random/lfg.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/json/document.hpp"

// Muddle
//#include "prestle.hpp"


#include "http/server.hpp"
#include "http/json_response.hpp"
#include "http/middleware/allow_origin.hpp"

#include "bootstrap_monitor.hpp"
#include "core/commandline/vt100.hpp"
#include <fstream>
//using namespace fetch;
//using namespace fetch::ledger;

using fetch::network::NetworkManager;
using fetch::network::Peer;
// using fetch::muddle::Muddle;
//using fetch::muddle::rpc::Client;
//using fetch::service::Promise;
using fetch::commandline::ParamsParser;

int main() 
{
  std::cout << "Hello world" << std::endl;
  fetch::ledger::LeaderBoard tracker;

  std::vector< std::unique_ptr<fetch::crypto::ECDSASigner> > peers;
  std::vector< typename fetch::ledger::LeaderBoard::Candidacy > cands;
  for(std::size_t i = 0; i < 20; ++i)
  {
    auto s = std::make_unique<fetch::crypto::ECDSASigner>() ;
    cands.push_back( {s->identity(), 3} );
    peers.push_back( std::move(s) );
  }


  std::vector< uint64_t > history;
  tracker.Setup({128831, 86942827}, cands);

  std::cout << "Candidates count: " << tracker.candidates().size() << std::endl;
  std::cout << "Leader count: " << tracker.size() << std::endl;


  std::size_t i = 0;

  for(; i < 4; ++i)
  {


    if(tracker.AdvanceToNextLeader())
    {
      std::cout << "Round " << (i >> 1) << std::endl;
      for(auto const &l: tracker.leaders())
      {
        std::cout << l.round << ": " << fetch::byte_array::ToBase64(l.identity.identifier()) << std::endl;
      }

      std::cout << "ADVANCE TO NEXT ROUND" << std::endl;
      history.push_back(tracker.random_number());
      tracker.Forward(i * 1337, {}, {});
    }
  }


  std::cout << "XXXXXXX" << std::endl;

  while(history.size() != 0)
  {
    tracker.Backward(history.back(), {}, {});
    std::cout << "Round " << history.size() << std::endl;
    for(auto const &l: tracker.leaders())
    {
      std::cout << l.round << ": " << fetch::byte_array::ToBase64(l.identity.identifier()) << std::endl;
    }
    history.pop_back();
  }

  return 0;
}

/*
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


  if(doc.HasKey("public_key"))
  {
    std::cout << "Setting key from config" << std::endl;
    if(!doc.HasKey("private_key"))
    {
      std::cerr << "please specify private key or delete public key" << std::endl;
      exit(-1);
    }

    fetch::byte_array::ByteArray public_key = doc["public_key"].As<fetch::byte_array::ByteArray>();
    fetch::byte_array::ByteArray private_key = doc["private_key"].As<fetch::byte_array::ByteArray>();
    signer->Load(fetch::byte_array::FromBase64(private_key)); 
    certificate->Load(fetch::byte_array::FromBase64(private_key));  
  }
  else
  {
    signer->GenerateKeys();
    certificate->Load( signer->private_key().Copy() );
  }


  // Setting  up
  NetworkManager network_manager( params.GetParam<std::size_t>("threads", 2) );
  network_manager.Start();
  std::cout << "Node certificiate: " << fetch::byte_array::ToBase64(signer->identity().identifier()) << std::endl;

  uint16_t port = params.GetParam<uint16_t>("port",0);
  if(port == 0)
  {
    port = doc["port"].As<uint16_t>();
  }



  // Starting

  std::cout << "Listening on " << port << std::endl;

  Muddle muddle{std::move(signer), network_manager};
  std::vector< fetch::network::Uri > connect_to;
  auto peers = doc["peers"];
  for(std::size_t i=0; i < peers.size(); ++i)
  {
    std::cout << i <<") " << peers[i].As<fetch::byte_array::ByteArray>() << std::endl;
    connect_to.push_back( fetch::network::Uri(std::string(peers[i].As<fetch::byte_array::ByteArray>()))  );
  }

  // Boostrap monitor
  uint32_t network_id = 9836122;
  std::string bootstrap_token = "f073c9ab5893eff21f2c70e71ae325b35bbd55af";  
  std::string hostname = "bootstrap.economicagents.com";
  auto bootstrap_monitor = std::make_unique<fetch::BootstrapMonitor>(certificate->identity(), port,
                                                                      network_id, bootstrap_token, hostname);
  bootstrap_monitor->Start(connect_to);  
  std::cout << "Interface address: " << bootstrap_monitor->interface_address() << std::endl;
  std::cout << "External address: " << bootstrap_monitor->external_address() << std::endl;



  muddle.Start({port}, connect_to);

  // Creating the consensus controller
  auto committee = doc["committee"];

  fetch::Pestle controller(muddle);
  controller.SetCertificate( certificate->private_key() );  
  controller.Setup();

  for(std::size_t i=0; i < committee.size(); ++i)
  {    
    auto param = committee[i]["parameters"].As<fetch::byte_array::ByteArray>();
    auto pk = committee[i]["key"].As<fetch::byte_array::ByteArray>();
    std::cout << "Registering candidate: "  << pk << std::endl;

    fetch::crypto::Identity identity{ param, fetch::byte_array::FromBase64(pk) };
    controller.RegisterCandidacy(identity, 5);
  }

  controller.PushRandomNumber(1337);
  controller.PushRandomNumber(42);  
  controller.NextRound();

  std::string            line = "";
  fetch::random::LaggedFibonacciGenerator<> lfg;


  // HTTP Server
  fetch::http::HTTPServer http_server(network_manager);
  uint16_t http_port = params.GetParam< uint16_t >("http_port", 0);
  if(http_port == 0)
  {
    http_port = doc["http_port"].As<uint16_t>();
  }
  http_server.Start( http_port);
  http_server.AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
  http_server.AddView(fetch::http::Method::GET, "/dag", [&controller](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
    std::stringstream ret("");
    ret << "[";
    bool first = true;
//    std::cout << "GETTING HTTP" << std::endl;
    for(auto const &n: controller.DAGNodes()) 
    {
      if(!first)
      {
        ret << ", ";
      }
      ret << "{";
      ret << "  \"type\": " << n.type << ",";         
      ret << "  \"contents\": \"" << n.contents << "\",";       
      ret << "  \"time\": \"" << n.timestamp << "\",";           
      ret << "  \"hash\": \"" << fetch::byte_array::ToBase64(n.hash) << "\",";
      ret << "  \"pk\": \"" << fetch::byte_array::ToBase64(n.identity.identifier()) << "\",";      
      ret << "  \"previous\": [";
      bool bfirst = true;
      for(auto const &p: n.previous)
      {
        if(!bfirst) ret << ",";
        ret << "\"" << fetch::byte_array::ToBase64(p) << "\"";
        bfirst = false;
      }
      ret << "]";
      ret << "}";

      first = false;
    }
    ret << "]";
//    std::cout << "DONE HTTP" << std::endl;    
    return fetch::http::CreateJsonResponse(ret.str());
  });  


  // Main loop
  std::cout << ">> ";
  std::getline(std::cin, line);

  while ((std::cin) && (line != "quit"))
  {
    // Getting command

    fetch::ledger::DAGNode node;

    // TODO: Set previous and type
    std::vector< fetch::byte_array::ConstByteArray > prev_candidates;
    for(auto &d: controller.last_nodes())
    {
      prev_candidates.push_back(d);
    }

    node.contents = line;
    node.identity = certificate->identity();

    if(prev_candidates.size() > 0 )
    {
      node.previous.push_back( prev_candidates[ lfg() % prev_candidates.size() ]);
    } 

    if(prev_candidates.size() > 3 )
    {
      node.previous.push_back( prev_candidates[ lfg() % prev_candidates.size() ]);
    }
   
//    node.previous.push_back( prev_candidates[ lfg() % prev_candidates.size() ]);    

    node.Finalise();


    if(!certificate->Sign(node.hash))
    {
      std::cerr << "Failed signing." << std::endl;
      continue;
    }

    node.signature = certificate->signature();


//    std::cout << fetch::byte_array::ToBase64(node.signature) << std::endl;
//    std::cout << fetch::byte_array::ToBase64(node.hash) << std::endl;    
//    std::cout << fetch::byte_array::ToBase64(node.identity.identifier()) << std::endl; 

    fetch::crypto::ECDSAVerifier verifier(node.identity);
    if(!verifier.Verify(node.hash, node.signature))
    {
      std::cerr << "NOT VERIFIABLE" << std::endl;
    }

    // Serializing
    fetch::serializers::TypedByteArrayBuffer buf;
    buf << node;

    controller.RegisterGeneralNode(node);
    muddle.AsEndpoint().Broadcast(SERVICE_MUDDLE, CHANNEL_DAG, buf.data());


    // Printing stuff
    / *
    auto rngs = controller.random_history();
    uint16_t ln = 1;

    using fetch::commandline::VT100::ClearLine;
    using fetch::commandline::VT100::Goto;

    for(auto &r: rngs)
    {
      std::cout << Goto(1,ln) << ClearLine();
      std::cout  << ln << ": " << r;
      ++ln;
    }
    
    while(ln < 20)
    {
      std::cout << Goto(1,ln) << ClearLine();      
      std::cout  << ln << ": " ;
      ++ln;
    }

    std::cout << Goto(1,ln) << ClearLine();
    std::cout << Goto(1,ln+1) << ClearLine();    
* /
//    line = "hello world";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));


  }

  return 0;
}
*/
