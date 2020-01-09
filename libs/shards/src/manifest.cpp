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

#include "json/document.hpp"
#include "json/exceptions.hpp"
#include "logging/logging.hpp"
#include "shards/manifest.hpp"
#include "variant/variant_utils.hpp"

#include <stdexcept>

namespace fetch {
namespace shards {
namespace {

constexpr char const *LOGGING_NAME = "Manifest";

}  // namespace

std::size_t Manifest::size() const
{
  return service_map_.size();
}

Manifest::iterator Manifest::begin()
{
  return service_map_.begin();
}

Manifest::iterator Manifest::end()
{
  return service_map_.end();
}

Manifest::const_iterator Manifest::begin() const
{
  return service_map_.begin();
}

Manifest::const_iterator Manifest::end() const
{
  return service_map_.end();
}

Manifest::const_iterator Manifest::cbegin() const
{
  return service_map_.cbegin();
}
Manifest::const_iterator Manifest::cend() const
{
  return service_map_.cend();
}

Manifest::iterator Manifest::FindService(ServiceIdentifier const &service)
{
  return service_map_.find(service);
}

Manifest::const_iterator Manifest::FindService(ServiceIdentifier const &service) const
{
  return service_map_.find(service);
}

Manifest::const_iterator Manifest::FindService(ServiceIdentifier::Type service_type) const
{
  return service_map_.find(ServiceIdentifier{service_type});
}

void Manifest::AddService(ServiceIdentifier const &id, ManifestEntry const &entry)
{
  service_map_.emplace(id, entry);
}

void Manifest::AddService(ServiceIdentifier const &id, ManifestEntry &&entry)
{
  service_map_.emplace(id, std::move(entry));
}

std::string Manifest::FindExternalAddress(ServiceIdentifier::Type type, uint32_t index) const
{
  auto it = FindService(ServiceIdentifier{type, index});
  if (it == end())
  {
    throw std::runtime_error(std::string{"Unable to look up external address for "} +
                             ToString(type));
  }

  return it->second.uri().GetTcpPeer().address();
}

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
      if (ExtractSection(doc["p2p"], ServiceIdentifier{ServiceIdentifier::Type::CORE}) &&
          ExtractSection(doc["http"], ServiceIdentifier{ServiceIdentifier::Type::HTTP}) &&
          ExtractSection(doc["dkg"], ServiceIdentifier{ServiceIdentifier::Type::DKG}))
      {
        auto lanes = doc["lanes"];

        // sanity check the type of the variant
        if (lanes.IsArray())
        {
          bool failures = false;

          // loop through all the lane services
          for (std::size_t i = 0, end = lanes.size(); i < end; ++i)
          {
            // create the lane service id
            ServiceIdentifier const lane_service{ServiceIdentifier::Type::LANE,
                                                 static_cast<uint32_t>(i)};

            // attempt to extract the section of the config
            if (!ExtractSection(lanes[i], lane_service))
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
  catch (json::JSONParseException const &ex)
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

bool Manifest::ExtractSection(Variant const &obj, ServiceIdentifier const &service)
{
  bool success = false;

  // ensure things are as we expect
  if (obj.IsObject())
  {
    ConstByteArray uri_str{};
    uint16_t       port{0};

    bool const has_uri  = variant::Extract(obj, "uri", uri_str);
    bool const has_port = variant::Extract(obj, "port", port);

    if (!has_uri)
    {
      return false;
    }

    // parse the URI string
    network::Uri uri;
    if (!uri.Parse(uri_str))
    {
      return false;
    }

    // for the moment we only support TCP urls in the manifest
    if (!uri.IsTcpPeer())
    {
      return false;
    }

    // in the case where the port is not specified (and/or valid) default to the URI port
    if (has_port)
    {
      service_map_.emplace(service, ManifestEntry{uri, port});
    }
    else
    {
      service_map_.emplace(service, ManifestEntry{uri});
    }
    success = true;
  }

  return success;
}

}  // namespace shards
}  // namespace fetch
