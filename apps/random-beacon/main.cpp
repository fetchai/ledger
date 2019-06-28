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

#include "core/byte_array/byte_array.hpp"
#include "core/commandline/params.hpp"
#include "crypto/ecdsa.hpp"
#include "network/management/network_manager.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/muddle.hpp"
#include "dkg/dkg_service.hpp"

#include <iostream>
#include <chrono>
#include <memory>
#include <fstream>

using namespace std::literals::chrono_literals;

using std::this_thread::sleep_for;
using fetch::muddle::NetworkId;
using fetch::commandline::Params;
using fetch::muddle::Muddle;
using fetch::network::NetworkManager;
using fetch::crypto::ECDSASigner;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::dkg::DkgService;

using CertificatePtr = std::shared_ptr<ECDSASigner>;

static ConstByteArray LoadContents(char const *filename)
{
  ByteArray data{};

  std::ifstream key_stream{filename, std::ios::binary | std::ios::in | std::ios::ate};
  if (key_stream.is_open())
  {
    auto const size = key_stream.tellg();

    // resize the buffer
    data.Resize(static_cast<std::size_t>(size));

    // revert to beginning and read the entire contents
    key_stream.seekg(0, std::ios::beg);
    key_stream.read(data.char_pointer(), size);
  }

  return {data};
}

static CertificatePtr LoadCertificate()
{
  static char const *FILENAME = "network.key";

  auto const key_contents = LoadContents(FILENAME);

  CertificatePtr cert{};
  if (!key_contents.empty())
  {
    cert = std::make_shared<ECDSASigner>(key_contents);
  }
  else
  {
    // generate a new key
    cert = std::make_shared<ECDSASigner>();

    auto const private_key_bytes = cert->private_key();

    // flush the dsit
    std::ofstream key_stream{FILENAME, std::ios::out | std::ios::binary};
    key_stream.write(private_key_bytes.char_pointer(), static_cast<std::streamsize>(private_key_bytes.size()));
  }

  return cert;
}

int main(int argc, char **argv)
{
  uint16_t port{0};
  std::string peers{};
  std::string dealer_raw{};

  // parse the commandline
  Params params;
  params.add<uint16_t>(port, "port", "The port to bind to", 8000);
  params.add<std::string>(peers, "peers", "The initial peers to connect to", "");
  params.add<std::string>(dealer_raw, "dealer", "The identity of the dealer", "");
  params.Parse(argc, argv);

  ConstByteArray const dealer{dealer_raw};

  auto identity = LoadCertificate();

  std::cout << "Port....: " << port << std::endl;
  std::cout << "Peers...: " << peers << std::endl;
  std::cout << "Identity: " << identity->identity().identifier().ToBase64() << std::endl;
  std::cout << "Dealer..: " << dealer.ToBase64() << std::endl;

  NetworkManager nm{"main", 1};
  nm.Start();

  Muddle muddle{NetworkId{"DKG-"}, identity, nm, true, true};
  muddle.Start({port}, {});

  DkgService service{muddle.AsEndpoint(), identity->identity().identifier(), dealer, 10};

  uint64_t block_index{0};
  for (;;)
  {
    service.OnNewBlock(block_index);
    sleep_for(10s);
    ++block_index;
  }

  nm.Stop();


  return 0;
}