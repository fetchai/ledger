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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "muddle/network_id.hpp"
#include "muddle/packet.hpp"
#include "network/fetch_asio.hpp"
#include "network/tcp/client_implementation.hpp"

#include <iostream>
#include <string>

struct PingMessage
{
};

namespace fetch {
namespace serializers {

template <typename D>
struct MapSerializer<PingMessage, D>
{
public:
  using Type       = PingMessage;
  using DriverType = D;
  using EnumType   = uint64_t;

  static const uint8_t TYPE = 1;

  template <typename T>
  static void Serialize(T &map_constructor, Type const &msg)
  {
    FETCH_UNUSED(msg);

    auto map = map_constructor(1);
    map.Append(TYPE, uint64_t{0});
  }

  template <typename T>
  static void Deserialize(T &map, Type &msg)
  {
    FETCH_UNUSED(map);
    FETCH_UNUSED(msg);

    throw std::runtime_error("Not implemented");
  }
};

}  // namespace serializers
}  // namespace fetch

namespace {

using fetch::muddle::Packet;
using fetch::crypto::ECDSASigner;
using fetch::muddle::NetworkId;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::network::TCPClientImplementation;
using fetch::serializers::MsgPackSerializer;
using fetch::byte_array::FromHex;

using Socket    = asio::ip::tcp::socket;
using Resolver  = asio::ip::tcp::resolver;
using IoService = asio::io_service;

ByteArray FormatPacket(Packet const &packet)
{
  ByteArray buffer;

  // work out the
  std::size_t const packet_output_length = packet.GetPacketSize();

  // serialize the packet header to the buffer
  TCPClientImplementation::SetHeader(buffer, packet_output_length);

  std::size_t const header_length = buffer.size();
  std::size_t const total_length  = header_length + packet_output_length;

  // update the packet size
  buffer.Resize(total_length);
  if (!Packet::ToBuffer(packet, buffer.pointer() + header_length, packet_output_length))
  {
    throw std::runtime_error{"Unable to convert packet to a buffer"};
  }

  return buffer;
}

void PopulateBuffer(Socket &sock, ByteArray &buffer, std::error_code &ec)
{
  asio::read(sock, asio::buffer(buffer.pointer(), buffer.size()),
             asio::transfer_exactly(buffer.size()), ec);
}

bool ReadPacket(Packet &packet, Socket &sock, std::error_code &ec)
{
  static const std::size_t HEADER_LENGTH = 2 * sizeof(uint64_t);

  ByteArray header{};
  header.Resize(HEADER_LENGTH);

  // read the header from the wire
  PopulateBuffer(sock, header, ec);
  if (ec)
  {
    std::cerr << "Error populating header buffer: " << std::endl;
    return false;
  }

  // work out the size of the packet
  uint64_t const magic  = *reinterpret_cast<uint64_t const *>(header.pointer());
  uint64_t const length = *reinterpret_cast<uint64_t const *>(header.pointer() + sizeof(uint64_t));

  // check to see if the magic is correct
  if (magic != TCPClientImplementation::NETWORK_MAGIC)
  {
    std::cerr << "Error invalid magic" << std::endl;
    return false;
  }

  ByteArray data{};
  data.Resize(length);

  // recv. all the data
  PopulateBuffer(sock, data, ec);
  if (ec)
  {
    std::cerr << "Error populating data buffer: " << ec.message() << std::endl;
    return false;
  }

  return Packet::FromBuffer(packet, data.pointer(), data.size());
}

void WritePacket(Packet const &packet, Socket &sock, std::error_code &ec)
{
  // render the packet into a buffer
  ByteArray buffer = FormatPacket(packet);

  // write the data to the socket
  asio::write(sock, asio::buffer(buffer.pointer(), buffer.size()), ec);
  if (ec)
  {
    std::cerr << "Error writing data to socket: " << ec.message() << std::endl;
    return;
  }
}

std::error_code SendPingTo(std::string const &host, std::string const &port, uint32_t network_id)
{
  std::error_code ec;
  IoService       service{};

  // resolve the address
  Resolver resolver{service};
  auto     it = resolver.resolve({host, port}, ec);
  if (ec)
  {
    std::cerr << "Error writing resolving to host: " << ec.message() << std::endl;
    return ec;
  }

  // create the connection
  Socket socket{service};
  socket.connect(*it, ec);
  if (ec)
  {
    std::cerr << "Error writing connecting to host: " << ec.message() << std::endl;
    return ec;
  }

  // create an identity
  ECDSASigner signer{};

  // form the ping packet
  Packet packet{signer.identity().identifier(), network_id};
  packet.SetService(fetch::SERVICE_MUDDLE);
  packet.SetChannel(fetch::CHANNEL_ROUTING);
  packet.SetDirect(true);

  MsgPackSerializer msg{};
  msg << PingMessage{};
  packet.SetPayload(msg.data());

  packet.Sign(signer);

  // write the packet to the server
  WritePacket(packet, socket, ec);
  if (ec)
  {
    std::cerr << "Error writing ping packet: " << ec.message() << std::endl;
    return ec;
  }

  // read the packet back from the server
  bool const success = ReadPacket(packet, socket, ec);
  if (ec)
  {
    std::cerr << "Error reading packet: " << ec.message() << std::endl;
    return ec;
  }

  if (!success)
  {
    std::cerr << "Failed to read packet from server" << std::endl;
    return {};
  }

  std::cout << "Remote: " << packet.GetSender().ToBase64() << std::endl;

  return {};
}

bool IsLaneId(std::string const &name)
{
  if (name.size() != 7)
  {
    return false;
  }

  if (name[0] != 'L')
  {
    return false;
  }

  for (std::size_t i = 1; i < name.size(); ++i)
  {
    if (!((name[i] >= '0') && (name[i] <= '9')))
    {
      return false;
    }
  }

  return true;
}

uint32_t ConvertNetworkId(std::string const &name)
{
  uint32_t network_id{0};

  if ((name == "IHUB") || (name == "ISRD") || (name == "DKGN"))
  {
    network_id = NetworkId{name.c_str()}.value();
  }
  else if (IsLaneId(name))
  {
    auto const converted =
        FromHex(ConstByteArray{reinterpret_cast<uint8_t const *>(name.data() + 1), 6});

    network_id = uint32_t{'L'} << 24u;

    assert(converted.size() == 3);
    for (uint32_t i = 0; i < 3; ++i)
    {
      network_id |= (uint32_t{converted[i]} << ((3u - (i + 1u)) * 8u));
    }
  }

  return network_id;
}

}  // namespace

int main(int argc, char **argv)
{
  int exit_code{EXIT_FAILURE};

  try
  {
    if (argc != 4)
    {
      std::cerr << "Usage: " << argv[0] << " <host> <port> <network>" << std::endl;
      return EXIT_FAILURE;
    }

    // convert the network id
    auto const nid = ConvertNetworkId(argv[3]);
    if (nid == 0)
    {
      std::cerr << "Failed to convert network id: " << argv[3] << std::endl;
      return EXIT_FAILURE;
    }

    auto const ec = SendPingTo(argv[1], argv[2], nid);
    if (ec)
    {
      std::cerr << "Error: " << ec.message() << std::endl;
      return EXIT_FAILURE;
    }

    // great success
    exit_code = EXIT_SUCCESS;
  }
  catch (std::exception const &ex)
  {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
