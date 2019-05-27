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

#include "scope_client.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/service_ids.hpp"
#include "network/muddle/network_id.hpp"
#include "network/muddle/packet.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono;
using namespace std::this_thread;

using fetch::muddle::NetworkId;
using fetch::muddle::Packet;
using fetch::serializers::ByteArrayBuffer;

using Clock     = steady_clock;
using Timestamp = Clock::time_point;

ScopeClient::ScopeClient()
{
  manager_.Start();
}

ScopeClient::~ScopeClient()
{
  manager_.Stop();
}

void ScopeClient::Ping(ConstByteArray const &host, uint16_t port)
{
  // create the client
  CreateClient(host, port);

  if (WaitUntilConnected())
  {
    // send the message to the server
    Packet packet{};
    packet.SetDirect(true);
    packet.SetNetworkId(0);
    packet.SetService(fetch::SERVICE_MUDDLE);
    packet.SetProtocol(fetch::CHANNEL_PROBE);

    // send the packet to the client
    SendMessage(packet);

    // wait for a response
    RecvMessage(packet);

    std::cout << "Version   : " << static_cast<uint32_t>(packet.GetVersion()) << '\n';
    std::cout << "Network ID: " << NetworkId{packet.GetNetworkId()}.ToString() << '\n';
    std::cout << "Sender    : " << packet.GetSender().ToBase64() << '\n';
    std::cout << std::endl;
  }

  client_->Close();
  client_.reset();
}

void ScopeClient::CreateClient(ConstByteArray const &host, uint16_t port)
{
  if (client_)
  {
    throw std::runtime_error("Concurrent process in progress");
  }

  // create the client
  client_         = std::make_unique<TCPClient>(manager_);
  auto connection = client_->connection_pointer().lock();

  auto self = shared_from_this();

  connection->OnConnectionFailed([self]() { self->state_ = State::CONNECTION_FAILED; });

  connection->OnConnectionSuccess([self]() { self->state_ = State::CONNECTED; });

  connection->OnLeave([self]() { self->state_ = State::CONNECTION_CLOSED; });

  // add the messages directly to the queue
  connection->OnMessage([self](ByteArray buffer) { self->messages_.Push(std::move(buffer)); });

  // update the state and start connecting
  state_ = State::CONNECTING;
  client_->Connect(host, port);
}

bool ScopeClient::WaitUntilConnected()
{
  // compute the connection timeout
  Timestamp const deadline = Clock::now() + seconds{30};

  for (;;)
  {
    if (state_ != State::CONNECTING)
    {
      break;
    }

    if (Clock::now() >= deadline)
    {
      break;
    }

    // wait for a bit
    sleep_for(milliseconds{300});
  }

  return (state_ == State::CONNECTED);
}

template <typename T>
void ScopeClient::SendMessage(T const &packet)
{
  // serialise the buffer
  ByteArrayBuffer buffer{};
  buffer << packet;

  // dispatch the packet
  client_->Send(buffer.data());
}

template <typename T>
void ScopeClient::RecvMessage(T &packet)
{
  ByteArray message{};
  if (!messages_.Pop(message, seconds{4}))
  {
    throw std::runtime_error("Failed to recv message from the server");
  }

  // deserialise the message
  ByteArrayBuffer buffer{message};
  buffer >> packet;
}
