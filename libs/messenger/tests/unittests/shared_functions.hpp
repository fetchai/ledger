#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "http/json_client.hpp"
#include "http/server.hpp"
#include "messenger/messenger_api.hpp"
#include "messenger/messenger_http_interface.hpp"
#include "messenger/messenger_prototype.hpp"

#include <memory>

struct CertificateGenerator
{
  using Prover         = fetch::crypto::Prover;
  using ProverPtr      = std::shared_ptr<Prover>;
  using Certificate    = fetch::crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;

  static ProverPtr New()
  {
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
  }
};

class FakeMailbox : public fetch::messenger::MailboxInterface
{
public:
  using Address = fetch::muddle::Packet::Address;
  using Message = fetch::messenger::Message;

  void SetDeliveryFunction(DeliveryFunction const & /*attempt_delivery*/) override
  {}

  void SendMessage(Message /*message*/) override
  {
    ++send;
  }

  MessageList GetMessages(Address /*messenger*/) override
  {
    ++empty_mailbox;
    return {};
  }

  void ClearMessages(Address /*messenger*/, uint64_t count) override
  {
    cleared += count;
  }

  void RegisterMailbox(Address /*messenger*/) override
  {
    ++registered_messengers;
  }

  void UnregisterMailbox(Address /*messenger*/) override
  {
    ++unregistered_messengers;
  }

  std::atomic<uint64_t> send{0};
  std::atomic<uint64_t> empty_mailbox{0};
  std::atomic<uint64_t> cleared{0};
  std::atomic<uint64_t> registered_messengers{0};
  std::atomic<uint64_t> unregistered_messengers{0};
};

struct ServerWithFakeMailbox
{
  using Prover              = fetch::crypto::Prover;
  using ProverPtr           = std::shared_ptr<Prover>;
  using Certificate         = fetch::crypto::Prover;
  using CertificatePtr      = std::shared_ptr<Certificate>;
  using Address             = fetch::muddle::Packet::Address;
  using NetworkManager      = fetch::network::NetworkManager;
  using MuddlePtr           = fetch::muddle::MuddlePtr;
  using MessengerAPI        = fetch::messenger::MessengerAPI;
  using MessengerPrototype  = fetch::messenger::MessengerPrototype;
  using Mailbox             = fetch::messenger::Mailbox;
  using Message             = fetch::messenger::Message;
  using MessageList         = fetch::messenger::Mailbox::MessageList;
  using HTTPServer          = fetch::http::HTTPServer;
  using MessengerHttpModule = fetch::messenger::MessengerHttpModule;
  using JsonClient          = fetch::http::JsonClient;
  using SharedJsonClient    = std::shared_ptr<JsonClient>;
  using Variant             = fetch::variant::Variant;
  using ConstByteArray      = fetch::byte_array::ConstByteArray;
  using MsgPackSerializer   = fetch::serializers::MsgPackSerializer;

  explicit ServerWithFakeMailbox(uint16_t port_offset)
    : certificate{CertificateGenerator::New()}
    , network_manager{"SearchNetworkManager", 1}
    , messenger_muddle{fetch::muddle::CreateMuddle("MSGN", certificate, network_manager,
                                                   "127.0.0.1")}
    , api{messenger_muddle, mailbox}
    , http{network_manager}
    , http_module{api}
  {
    network_manager.Start();
    messenger_muddle->Start({static_cast<uint16_t>(1337 + port_offset)});

    http.AddModule(http_module);
    http.Start(static_cast<uint16_t>(8000 + port_offset));
  }

  ProverPtr      certificate;
  NetworkManager network_manager;
  MuddlePtr      messenger_muddle;
  FakeMailbox    mailbox;
  MessengerAPI   api;

  HTTPServer          http;
  MessengerHttpModule http_module;
};

struct Server
{
  using Prover              = fetch::crypto::Prover;
  using ProverPtr           = std::shared_ptr<Prover>;
  using Certificate         = fetch::crypto::Prover;
  using CertificatePtr      = std::shared_ptr<Certificate>;
  using Address             = fetch::muddle::Packet::Address;
  using NetworkManager      = fetch::network::NetworkManager;
  using MuddlePtr           = fetch::muddle::MuddlePtr;
  using MessengerAPI        = fetch::messenger::MessengerAPI;
  using MessengerPrototype  = fetch::messenger::MessengerPrototype;
  using Mailbox             = fetch::messenger::Mailbox;
  using Message             = fetch::messenger::Message;
  using MessageList         = fetch::messenger::Mailbox::MessageList;
  using HTTPServer          = fetch::http::HTTPServer;
  using MessengerHttpModule = fetch::messenger::MessengerHttpModule;
  using JsonClient          = fetch::http::JsonClient;
  using SharedJsonClient    = std::shared_ptr<JsonClient>;
  using Variant             = fetch::variant::Variant;
  using ConstByteArray      = fetch::byte_array::ConstByteArray;
  using MsgPackSerializer   = fetch::serializers::MsgPackSerializer;

  explicit Server(uint16_t port_offset)
    : certificate{CertificateGenerator::New()}
    , network_manager{"SearchNetworkManager", 1}
    , messenger_muddle{fetch::muddle::CreateMuddle("MSGN", certificate, network_manager,
                                                   "127.0.0.1")}
    , mail_muddle{fetch::muddle::CreateMuddle("MALM", certificate, network_manager, "127.0.0.1")}
    , mailbox{mail_muddle}
    , api{messenger_muddle, mailbox}
    , http{network_manager}
    , http_module{api}
  {
    network_manager.Start();
    messenger_muddle->Start({static_cast<uint16_t>(1337 + port_offset)});

    mail_muddle->SetTrackerConfiguration(fetch::muddle::TrackerConfiguration::AllOn());
    mail_muddle->Start({static_cast<uint16_t>(6500 + port_offset)});

    http.AddModule(http_module);
    http.Start(static_cast<uint16_t>(8000 + port_offset));
  }

  ProverPtr      certificate;
  NetworkManager network_manager;
  MuddlePtr      messenger_muddle;
  MuddlePtr      mail_muddle;
  Mailbox        mailbox;
  MessengerAPI   api;

  HTTPServer          http;
  MessengerHttpModule http_module;
};

struct Messenger
{
  using Prover              = fetch::crypto::Prover;
  using ProverPtr           = std::shared_ptr<Prover>;
  using Certificate         = fetch::crypto::Prover;
  using CertificatePtr      = std::shared_ptr<Certificate>;
  using Address             = fetch::muddle::Packet::Address;
  using NetworkManager      = fetch::network::NetworkManager;
  using MuddlePtr           = fetch::muddle::MuddlePtr;
  using MessengerAPI        = fetch::messenger::MessengerAPI;
  using MessengerPrototype  = fetch::messenger::MessengerPrototype;
  using Mailbox             = fetch::messenger::Mailbox;
  using Message             = fetch::messenger::Message;
  using MessageList         = fetch::messenger::Mailbox::MessageList;
  using HTTPServer          = fetch::http::HTTPServer;
  using MessengerHttpModule = fetch::messenger::MessengerHttpModule;
  using JsonClient          = fetch::http::JsonClient;
  using SharedJsonClient    = std::shared_ptr<JsonClient>;
  using Variant             = fetch::variant::Variant;
  using ConstByteArray      = fetch::byte_array::ConstByteArray;
  using MsgPackSerializer   = fetch::serializers::MsgPackSerializer;

  explicit Messenger(uint16_t port)
    : certificate{CertificateGenerator::New()}
    , network_manager{"MessengerNetworkManager", 1}
    , messenger_muddle{
          fetch::muddle::CreateMuddle("MSGN", certificate, network_manager, "127.0.0.1")}
  {
    network_manager.Start();
    messenger_muddle->Start({"tcp://127.0.0.1:" + std::to_string(port)}, {});

    while (messenger_muddle->GetDirectlyConnectedPeers().empty())
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

inline std::shared_ptr<Messenger> NewMessenger(uint16_t port)
{
  return std::make_shared<Messenger>(port);
}

struct HTTPMessenger
{
  using Prover              = fetch::crypto::Prover;
  using ProverPtr           = std::shared_ptr<Prover>;
  using Certificate         = fetch::crypto::Prover;
  using CertificatePtr      = std::shared_ptr<Certificate>;
  using Address             = fetch::muddle::Packet::Address;
  using NetworkManager      = fetch::network::NetworkManager;
  using MuddlePtr           = fetch::muddle::MuddlePtr;
  using MessengerAPI        = fetch::messenger::MessengerAPI;
  using MessengerPrototype  = fetch::messenger::MessengerPrototype;
  using Mailbox             = fetch::messenger::Mailbox;
  using Message             = fetch::messenger::Message;
  using MessageList         = fetch::messenger::Mailbox::MessageList;
  using HTTPServer          = fetch::http::HTTPServer;
  using MessengerHttpModule = fetch::messenger::MessengerHttpModule;
  using JsonClient          = fetch::http::JsonClient;
  using SharedJsonClient    = std::shared_ptr<JsonClient>;
  using Variant             = fetch::variant::Variant;
  using ConstByteArray      = fetch::byte_array::ConstByteArray;
  using MsgPackSerializer   = fetch::serializers::MsgPackSerializer;

  explicit HTTPMessenger(uint16_t port)
    : certificate{CertificateGenerator::New()}
    , client{std::make_shared<JsonClient>(JsonClient::ConnectionMode::HTTP, "127.0.0.1", port)}
  {}

  bool Register()
  {
    Variant result;
    Variant payload   = Variant::Object();
    payload["sender"] = fetch::byte_array::ToBase64(certificate->identity().identifier());
    client->Post("/api/messenger/register", payload, result);
    if (!result.IsObject())
    {
      return false;
    }

    return result["status"].As<ConstByteArray>() == "OK";
  }

  bool Unregister()
  {
    Variant result;
    Variant payload   = Variant::Object();
    payload["sender"] = fetch::byte_array::ToBase64(certificate->identity().identifier());
    client->Post("/api/messenger/unregister", payload, result);
    if (!result.IsObject())
    {
      return false;
    }
    return result["status"].As<ConstByteArray>() == "OK";
  }

  bool SendMessage(Message const &msg)
  {
    MsgPackSerializer buffer;

    buffer << msg;

    Variant result;
    Variant payload    = Variant::Object();
    payload["sender"]  = fetch::byte_array::ToBase64(certificate->identity().identifier());
    payload["message"] = fetch::byte_array::ToBase64(buffer.data());
    client->Post("/api/messenger/sendmessage", payload, result);
    if (!result.IsObject())
    {
      return false;
    }

    return result["status"].As<ConstByteArray>() == "OK";
  }

  MessageList GetMessages()
  {
    Variant result;
    Variant payload   = Variant::Object();
    payload["sender"] = fetch::byte_array::ToBase64(certificate->identity().identifier());
    client->Post("/api/messenger/getmessages", payload, result);
    if (!result.IsObject())
    {
      return {};
    }

    MsgPackSerializer buffer{
        fetch::byte_array::FromBase64(result["messages"].As<ConstByteArray>())};
    MessageList ret;
    buffer >> ret;

    return ret;
  }

  Address GetAddress()
  {
    return certificate->identity().identifier();
  }

  ProverPtr        certificate{nullptr};
  SharedJsonClient client{nullptr};
};

inline std::shared_ptr<ServerWithFakeMailbox> NewServerWithFakeMailbox(uint16_t port_offset)
{
  return std::make_shared<ServerWithFakeMailbox>(port_offset);
}

inline std::shared_ptr<Server> NewServer(uint16_t port_offset)
{
  return std::make_shared<Server>(port_offset);
}

inline std::shared_ptr<HTTPMessenger> NewHTTPMessenger(uint16_t port)
{
  return std::make_shared<HTTPMessenger>(port);
}

template <typename T>
std::set<T> ToSet(std::deque<T> const &d)
{
  return std::set<T>(d.begin(), d.end());
}
