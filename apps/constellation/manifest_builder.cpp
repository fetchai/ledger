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

#include "constants.hpp"
#include "manifest_builder.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"
#include "network/peer.hpp"
#include "settings.hpp"

namespace fetch {
namespace {

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;
using fetch::network::Manifest;
using fetch::network::Peer;
using fetch::network::ServiceIdentifier;
using fetch::network::ServiceType;
using fetch::network::Uri;

/**
 * Generate a default simple manifest for all the services provided
 *
 * @param external_address The externally visible address to communicate on
 * @param port The starting port for the services
 * @param num_lanes The number of lanes to be configured
 * @param manifest The output manifest to be populated
 */
void GenerateDefaultManifest(std::string const &external_address, uint16_t port, uint32_t num_lanes,
                             Manifest &manifest)
{
  Peer peer;

  // register the HTTP service
  peer.Update(external_address, port + HTTP_PORT_OFFSET);
  manifest.AddService(ServiceIdentifier{ServiceType::HTTP}, Manifest::Entry{Uri{peer}});

  // register the P2P service
  peer.Update(external_address, static_cast<uint16_t>(port + P2P_PORT_OFFSET));
  manifest.AddService(ServiceIdentifier{ServiceType::CORE}, Manifest::Entry{Uri{peer}});

  // register all of the lanes (storage shards)
  for (uint32_t i = 0; i < num_lanes; ++i)
  {
    peer.Update(external_address, static_cast<uint16_t>(port + STORAGE_PORT_OFFSET + (2 * i)));

    manifest.AddService(ServiceIdentifier{ServiceType::LANE, static_cast<uint16_t>(i)},
                        Manifest::Entry{Uri{peer}});
  }
}

/**
 * Create the manifest from a specified file path
 *
 * @param filename The filename to try and load
 * @param manifest The output manifest to be populated
 * @return true if successful, otherwise false
 */
bool LoadManifestFromFile(char const *filename, Manifest &manifest)
{
  ConstByteArray buffer = core::ReadContentsOfFile(filename);

  // check to see if the read failed
  if (buffer.size() == 0)
  {
    return false;
  }

  return manifest.Parse(buffer);
}

/**
 * Create the manifest from an environment variable based configuration
 *
 * @param manifest The manifest to be populated
 * @return true if successful, otherwise false
 */
bool LoadManifestFromEnvironment(Manifest &manifest)
{
  bool success{false};

  char const *manifest_data = std::getenv("CONSTELLATION_MANIFEST");
  if (manifest_data != nullptr)
  {
    success = manifest.Parse(FromBase64(manifest_data));
  }

  return success;
}

}  // namespace

/**
 * Attempt to generate the system manifest from the settings provided
 *
 * @param settings The settings supplied to the system
 * @param manifest The output manifest to be populated
 * @return true is successful, otherwise false
 */
bool BuildManifest(Settings const &settings, network::Manifest &manifest)
{
  bool success{false};

  // attempt to load an existing manifest from the specified sources
  success = LoadManifestFromEnvironment(manifest) ||
            LoadManifestFromFile(settings.config.value().c_str(), manifest);

  // in the case where the manifest was not specified from environment variables or config. We
  // need to supply a default configuration
  if (!success)
  {
    GenerateDefaultManifest(settings.external.value(), settings.port.value(),
                            settings.num_lanes.value(), manifest);

    success = true;
  }

  return success;
}

}  // namespace fetch
