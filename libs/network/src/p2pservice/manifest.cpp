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

#include "network/p2pservice/manifest.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "variant/variant_utils.hpp"

#include <sstream>
#include <stdexcept>

namespace fetch {
namespace network {
namespace {

bool ToStream(Manifest const &manifest, std::ostream &stream, ServiceType service,
              std::size_t idx = 0)
{
  bool success = false;

  ServiceIdentifier const identifier{service, static_cast<uint16_t>(idx)};

  // ensure the service is present
  if (manifest.HasService(identifier))
  {
    // extract the service
    auto const &entry = manifest.GetService(identifier);

    // update the stream
    stream << " - " << identifier.ToString() << ": " << entry.remote_uri.uri() << " ("
           << entry.local_port << ')' << '\n';

    success = true;
  }

  return success;
}

}  // namespace

Manifest::Entry::Entry(fetch::network::Uri uri)
  : remote_uri{std::move(uri)}
{
  if (remote_uri.scheme() != Uri::Scheme::Tcp)
  {
    throw std::runtime_error("Invalid URI format for manifest entry");
  }

  local_port = remote_uri.AsPeer().port();
}

Manifest::Entry::Entry(fetch::network::Uri uri, uint16_t port)
  : remote_uri{std::move(uri)}
  , local_port{port}
{}

/**
 * Parse the incoming manifest text (JSON)
 *
 * @param text The JSON encoded string
 * @return true if successful, otherwise false
 */
bool Manifest::Parse(ConstByteArray const &text)
{
  bool success = false;

  // clear any existing configuration
  service_map_.clear();

  json::JSONDocument doc;

  try
  {
    // attempt to parse the document
    doc.Parse(text);

    // extract the elements
    if (doc.root().IsObject())
    {
      // attempt to extract the main section of the manifest
      if (ExtractSection(doc["p2p"], ServiceType::CORE) &&
          ExtractSection(doc["http"], ServiceType::HTTP))
      {
        auto lanes = doc["lanes"];

        // sanity check the type of the variant
        if (lanes.IsArray())
        {
          bool failures = false;

          // loop through all the lane services
          for (std::size_t i = 0, end = lanes.size(); i < end; ++i)
          {
            // attempt to extract the section of the config
            if (!ExtractSection(lanes[i], ServiceType::LANE, i))
            {
              FETCH_LOG_WARN(LOGGING_NAME, "Unable to parse lane section ", i, " of manifest");

              failures = true;
              break;
            }
          }

          // update the status based on the results
          if (!failures)
          {
            success = true;
          }
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to parse CORE section of manifest");
      }
    }
  }
  catch (json::JSONParseException &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error when parsing manifest: ", ex.what());
  }

  // in the case of a failure clear out the invalid state
  if (!success)
  {
    service_map_.clear();
  }

  return success;
}

bool Manifest::ExtractSection(Variant const &obj, ServiceType service, std::size_t instance)
{
  bool success = false;

  // ensure things are as we expect
  if (obj.IsObject())
  {
    ConstByteArray uri_str;
    uint16_t       port;

    bool const has_uri  = variant::Extract(obj, "uri", uri_str);
    bool const has_port = variant::Extract(obj, "port", port);

    if (!has_uri)
    {
      return false;
    }

    // parse the URI string
    Uri uri;
    if (!uri.Parse(uri_str))
    {
      return false;
    }

    // for the moment we only support TCP urls in the manifest
    if (uri.scheme() != Uri::Scheme::Tcp)
    {
      return false;
    }

    auto const &peer = uri.AsPeer();

    // in the case where the port is not specified (and/or valid) default to the URI port
    if (!has_port)
    {
      port = peer.port();
    }

    // add the entry into the map
    service_map_.emplace(ServiceIdentifier{service, static_cast<uint16_t>(instance)},
                         Entry{uri, port});

    success = true;
  }

  return success;
}

std::string Manifest::ToString() const
{
  std::ostringstream oss;
  oss << '\n';

  ToStream(*this, oss, ServiceType::HTTP);
  ToStream(*this, oss, ServiceType::CORE);

  for (std::size_t lane_index = 0;; ++lane_index)
  {
    // stop on the first lane which is not found
    if (!ToStream(*this, oss, ServiceType::LANE, lane_index))
    {
      break;
    }
  }

  return oss.str();
}

}  // namespace network
}  // namespace fetch
