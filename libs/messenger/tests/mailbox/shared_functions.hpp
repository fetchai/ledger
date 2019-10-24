#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "crypto/ecdsa.hpp"
#include "messenger/messenger_api.hpp"
#include "messenger/messenger_prototype.hpp"

#include <memory>

using Prover             = fetch::crypto::Prover;
using ProverPtr          = std::shared_ptr<Prover>;
using Certificate        = fetch::crypto::Prover;
using CertificatePtr     = std::shared_ptr<Certificate>;
using Address            = fetch::muddle::Packet::Address;
using NetworkManager     = fetch::network::NetworkManager;
using MuddlePtr          = fetch::muddle::MuddlePtr;
using MessengerAPI       = fetch::messenger::MessengerAPI;
using MessengerPrototype = fetch::messenger::MessengerPrototype;
using Mailbox            = fetch::messenger::Mailbox;
using Message            = fetch::messenger::Message;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

class FakeMailbox : public fetch::messenger::MailboxInterface
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

  void ClearMessages(Address /*messenger*/, uint64_t /*count*/) override
  {}

  void RegisterMailbox(Address) override
  {
    ++registered_messengers;
  }

  void UnregisterMailbox(Address) override
  {
    ++unregistered_messengers;
  }

  std::atomic<uint64_t> send{0};
  std::atomic<uint64_t> empty_mailbox{0};
  std::atomic<uint64_t> registered_messengers{0};
  std::atomic<uint64_t> unregistered_messengers{0};
};

struct ServerWithFakeMailbox
{
  ServerWithFakeMailbox(uint16_t port1, uint16_t)
    : certificate{CreateNewCertificate()}
    , network_manager{"SearchNetworkManager", 1}
    , messenger_muddle{fetch::muddle::CreateMuddle("MSGN", certificate, network_manager,
                                                   "127.0.0.1")}
    , api{messenger_muddle, mailbox}
  {
    network_manager.Start();
    messenger_muddle->Start({port1});
  }

  ProverPtr      certificate;
  NetworkManager network_manager;
  MuddlePtr      messenger_muddle;
  FakeMailbox    mailbox;
  MessengerAPI   api;
};

struct Server
{
  Server(uint16_t port1, uint16_t port2)
    : certificate{CreateNewCertificate()}
    , network_manager{"SearchNetworkManager", 1}
    , messenger_muddle{fetch::muddle::CreateMuddle("MSGN", certificate, network_manager,
                                                   "127.0.0.1")}
    , mail_muddle{fetch::muddle::CreateMuddle("XXXX", certificate, network_manager, "127.0.0.1")}
    , mailbox{mail_muddle}
    , api{messenger_muddle, mailbox}
  {
    network_manager.Start();
    messenger_muddle->Start({port1});
    mail_muddle->Start({port2});
  }

  ProverPtr      certificate;
  NetworkManager network_manager;
  MuddlePtr      messenger_muddle;
  MuddlePtr      mail_muddle;
  Mailbox        mailbox;
  MessengerAPI   api;
};

std::shared_ptr<ServerWithFakeMailbox> NewServerWithFakeMailbox(uint16_t port1, uint16_t port2)
{
  return std::make_shared<ServerWithFakeMailbox>(port1, port2);
}

std::shared_ptr<Server> NewServer(uint16_t port1, uint16_t port2)
{
  return std::make_shared<Server>(port1, port2);
}

struct Messenger
{
  Messenger(uint16_t port)
    : certificate{CreateNewCertificate()}
    , network_manager{"MessengerNetworkManager", 1}
    , messenger_muddle{
          fetch::muddle::CreateMuddle("MSGN", certificate, network_manager, "127.0.0.1")}
  {
    network_manager.Start();
    messenger_muddle->Start({"tcp://127.0.0.1:" + std::to_string(port)}, {});

    while (messenger_muddle->GetDirectlyConnectedPeers().size() < 1)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto messenger_api_addresses = messenger_muddle->GetDirectlyConnectedPeers();
    messenger = std::make_shared<MessengerPrototype>(messenger_muddle, messenger_api_addresses);
  }

  ProverPtr                           certificate;
  NetworkManager                      network_manager;
  MuddlePtr                           messenger_muddle;
  std::shared_ptr<MessengerPrototype> messenger;
};

std::shared_ptr<Messenger> NewMessenger(uint16_t port)
{
  return std::make_shared<Messenger>(port);
}

template <typename T>
std::set<T> ToSet(std::deque<T> const &d)
{
  return std::set<T>(d.begin(), d.end());
}
