
#include "agentapi/agent_prototype.hpp"
#include "agentapi/agentapi.hpp"
#include "crypto/ecdsa.hpp"

#include <memory>

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;
using NetworkManager = fetch::network::NetworkManager;
using MuddlePtr      = fetch::muddle::MuddlePtr;
using AgentAPI       = fetch::agent::AgentAPI;
using AgentPrototype = fetch::agent::AgentPrototype;
using Mailbox        = fetch::agent::Mailbox;
using Message        = fetch::agent::Message;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

class FakeMailbox : public fetch::agent::MailboxInterface
{
public:
  void SendMessage(Message) override
  {
    ++send;
  }

  MessageList GetMessages(Address) override
  {
    ++empty_mailbox;
    return {};
  }

  void RegisterMailbox(Address) override
  {
    std::cout << "Registering" << std::endl;
    ++registered_agents;
  }

  void UnregisterMailbox(Address) override
  {
    ++unregistered_agents;
  }

  std::atomic<uint64_t> send{0};
  std::atomic<uint64_t> empty_mailbox{0};
  std::atomic<uint64_t> registered_agents{0};
  std::atomic<uint64_t> unregistered_agents{0};
};

struct ServerWithFakeMailbox
{
  ServerWithFakeMailbox(uint16_t port1, uint16_t)
    : certificate{CreateNewCertificate()}
    , network_manager{"SearchNetworkManager", 1}
    , agent_muddle{fetch::muddle::CreateMuddle("AGEN", certificate, network_manager, "127.0.0.1")}
    , api{agent_muddle, mailbox}
  {
    network_manager.Start();
    agent_muddle->Start({port1});
  }

  ProverPtr      certificate;
  NetworkManager network_manager;
  MuddlePtr      agent_muddle;
  FakeMailbox    mailbox;
  AgentAPI       api;
};

std::shared_ptr<ServerWithFakeMailbox> NewServerWithFakeMailbox(uint16_t port1, uint16_t port2)
{
  return std::make_shared<ServerWithFakeMailbox>(port1, port2);
}

struct Server
{
  Server(uint16_t port1, uint16_t port2)
    : certificate{CreateNewCertificate()}
    , network_manager{"SearchNetworkManager", 1}
    , agent_muddle{fetch::muddle::CreateMuddle("AGEN", certificate, network_manager, "127.0.0.1")}
    , mail_muddle{fetch::muddle::CreateMuddle("XXXX", certificate, network_manager, "127.0.0.1")}
    , mailbox{mail_muddle}
    , api{agent_muddle, mailbox}
  {
    network_manager.Start();
    agent_muddle->Start({port1});
    mail_muddle->Start({port2});
  }

  ProverPtr      certificate;
  NetworkManager network_manager;
  MuddlePtr      agent_muddle;
  MuddlePtr      mail_muddle;
  Mailbox        mailbox;
  AgentAPI       api;
};

std::shared_ptr<Server> NewServer(uint16_t port1, uint16_t port2)
{
  return std::make_shared<Server>(port1, port2);
}

struct Agent
{
  Agent(uint16_t port)
    : certificate{CreateNewCertificate()}
    , network_manager{"AgentNetworkManager", 1}
    , agent_muddle{fetch::muddle::CreateMuddle("AGEN", certificate, network_manager, "127.0.0.1")}
  {
    network_manager.Start();
    agent_muddle->Start({"tcp://127.0.0.1:" + std::to_string(port)}, {});

    while (agent_muddle->GetDirectlyConnectedPeers().size() < 1)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto agent_api_addresses = agent_muddle->GetDirectlyConnectedPeers();
    agent                    = std::make_shared<AgentPrototype>(agent_muddle, agent_api_addresses);
  }

  ProverPtr                       certificate;
  NetworkManager                  network_manager;
  MuddlePtr                       agent_muddle;
  std::shared_ptr<AgentPrototype> agent;
};

std::shared_ptr<Agent> NewAgent(uint16_t port)
{
  return std::make_shared<Agent>(port);
}
