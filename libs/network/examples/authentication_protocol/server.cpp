//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "network/service/server.hpp"
#include "node_details.hpp"
#include "service_consts.hpp"
#include <iostream>
#include <memory>
#include <utility>
using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;

template <typename D>
class AuthenticationLogic
{
public:
  using node_details_type = D;

  AuthenticationLogic(fetch::network::ConnectionRegister<D> reg) : register_(std::move(reg)) {}

  uint64_t Ping() { return 1337; }

  byte_array::ByteArray Hello(network::AbstractConnection::connection_handle_type const &client)
  {
    auto details = register_.GetDetails(client);
    {
      std::lock_guard<mutex::Mutex> lock(*details);
      details->authenticated = true;
    }

    return byte_array::ByteArray();
  }

  byte_array::ByteArray GetChallenge(
      network::AbstractConnection::connection_handle_type const &client)
  {
    return byte_array::ByteArray();
  }

  void RespondToChallenge(network::AbstractConnection::connection_handle_type const &client,
                          byte_array::ByteArray const &                              response)
  {}

private:
  fetch::network::ConnectionRegister<D> register_;
};

template <typename D>
class AuthenticationProtocol : public Protocol
{
public:
  AuthenticationProtocol(AuthenticationLogic<D> *auth_logic) : Protocol()
  {
    this->Expose(PING, auth_logic, &AuthenticationLogic<D>::Ping);
    this->ExposeWithClientArg(HELLO, auth_logic, &AuthenticationLogic<D>::Hello);
    this->ExposeWithClientArg(GET_CHALLENGE, auth_logic, &AuthenticationLogic<D>::GetChallenge);
    this->ExposeWithClientArg(RESPOND_TO_CHALLENGE, auth_logic,
                              &AuthenticationLogic<D>::RespondToChallenge);
  }
};

class TestLogic
{
public:
  std::string Greet(std::string const &name) { return "Hello, " + name; }

  int Add(int a, int b) { return a + b; }
};

class TestProtocol : public Protocol
{
public:
  TestProtocol() : Protocol()
  {
    this->Expose(GREET, &test_, &TestLogic::Greet);
    this->Expose(ADD, &test_, &TestLogic::Add);
  }

private:
  TestLogic test_;
};

class ProtectedService : public ServiceServer<fetch::network::TCPServer>
{
public:
  ProtectedService(uint16_t port, fetch::network::NetworkManager tm) : ServiceServer(port, tm)
  {
    this->SetConnectionRegister(register_);

    auth_logic_ = std::make_unique<AuthenticationLogic<fetch::NodeDetails>>(register_);
    auth_proto_ = std::make_unique<AuthenticationProtocol<fetch::NodeDetails>>(auth_logic_.get());

    test_proto_.AddMiddleware([this](network::AbstractConnection::connection_handle_type const &n,
                                     byte_array::ByteArray const &data) {
      auto details = register_.GetDetails(n);
      bool authed  = false;
      {
        std::lock_guard<mutex::Mutex> lock(*details);
        authed = details->authenticated;
      }

      std::cout << "Is authenticated? " << (authed ? "YES" : "NO") << std::endl;

      if (!authed)
      {
        throw serializers::SerializableException(
            0, byte_array_type("Please authenticate by using the AUTH protocol."));
      }
    });

    this->Add(AUTH, auth_proto_.get());
    this->Add(TEST, &test_proto_);
  }

private:
  fetch::network::ConnectionRegister<fetch::NodeDetails>      register_;
  std::unique_ptr<AuthenticationLogic<fetch::NodeDetails>>    auth_logic_;
  std::unique_ptr<AuthenticationProtocol<fetch::NodeDetails>> auth_proto_;
  TestProtocol                                                test_proto_;
};

int main()
{
  fetch::network::NetworkManager tm(8);
  ProtectedService               serv(8080, tm);
  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
