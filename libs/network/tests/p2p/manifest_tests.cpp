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

#include <gtest/gtest.h>
#include <vector>

namespace {

struct ServiceData
{
  std::string address;
  uint16_t    remote_port;
  uint16_t    local_port;
};

struct TestCase
{
  char const *             name;
  char const *             text;
  ServiceData              http;
  ServiceData              p2p;
  std::vector<ServiceData> lanes;
};

std::ostream &operator<<(std::ostream &s, TestCase const &config)
{
  s << config.name;
  return s;
}

static TestCase const TEST_CASES[] = {{"Fully explicit configuration",
                                       R"(
    {
      "http": { "uri": "tcp://127.0.0.1:8000", "port": 8000 },
      "p2p": { "uri": "tcp://127.0.0.1:8001", "port": 8001 },
      "lanes": [
        { "uri": "tcp://127.0.0.1:8010", "port": 8010 },
        { "uri": "tcp://127.0.0.1:8011", "port": 8011 },
        { "uri": "tcp://127.0.0.1:8012", "port": 8012 },
        { "uri": "tcp://127.0.0.1:8013", "port": 8013 }
      ]
    }
    )",
                                       {"127.0.0.1", 8000, 8000},  // HTTP
                                       {"127.0.0.1", 8001, 8001},  // CORE
                                       {                           // LANES
                                        {"127.0.0.1", 8010, 8010},
                                        {"127.0.0.1", 8011, 8011},
                                        {"127.0.0.1", 8012, 8012},
                                        {"127.0.0.1", 8013, 8013}}},
                                      {"Mix of configurations",
                                       R"(
    {
      "http": { "uri": "tcp://192.168.1.54:30000", "port": 9000 },
      "p2p": { "uri": "tcp://192.168.1.55:30001", "port": 9001 },
      "lanes": [
        { "uri": "tcp://192.168.1.60:30100", "port": 9010 },
        { "uri": "tcp://192.168.1.61:30101", "port": 9011 },
        { "uri": "tcp://192.168.1.62:30102", "port": 9012 },
        { "uri": "tcp://192.168.1.63:30103", "port": 9013 }
      ]
    }
    )",
                                       {"192.168.1.54", 30000, 9000},  // HTTP
                                       {"192.168.1.55", 30001, 9001},  // CORE
                                       {                               // LANES
                                        {"192.168.1.60", 30100, 9010},
                                        {"192.168.1.61", 30101, 9011},
                                        {"192.168.1.62", 30102, 9012},
                                        {"192.168.1.63", 30103, 9013}}},
                                      {"Fully implicit configuration",
                                       R"(
    {
      "http": { "uri": "tcp://127.0.10.1:8000" },
      "p2p": { "uri": "tcp://127.0.0.1:8001" },
      "lanes": [
        { "uri": "tcp://127.1.0.1:8010" },
        { "uri": "tcp://127.2.0.1:8011" },
        { "uri": "tcp://127.3.0.1:8012" },
        { "uri": "tcp://127.4.0.1:8013" }
      ]
    }
    )",
                                       {"127.0.10.1", 8000, 8000},  // HTTP
                                       {"127.0.0.1", 8001, 8001},   // CORE
                                       {                            // LANES
                                        {"127.1.0.1", 8010, 8010},
                                        {"127.2.0.1", 8011, 8011},
                                        {"127.3.0.1", 8012, 8012},
                                        {"127.4.0.1", 8013, 8013}}}};

class ManifestTests : public ::testing::TestWithParam<TestCase>
{
protected:
  using ServiceIdentifier = fetch::network::ServiceIdentifier;
  using ServiceType       = fetch::network::ServiceType;
  using Manifest          = fetch::network::Manifest;
  using ManifestPtr       = std::unique_ptr<Manifest>;

  void SetUp() override
  {
    manifest_ = std::make_unique<Manifest>();
  }

  ManifestPtr manifest_;
};

TEST_P(ManifestTests, Check)
{
  TestCase const &config = GetParam();

  ASSERT_TRUE(manifest_->Parse(config.text));

  // check the p2p service
  ASSERT_TRUE(manifest_->HasService(ServiceIdentifier{ServiceType::CORE}));

  auto const &p2p_service = manifest_->GetService(ServiceIdentifier{ServiceType::CORE});
  auto const &p2p_peer    = p2p_service.remote_uri.AsPeer();

  EXPECT_EQ(p2p_peer.address(), config.p2p.address);
  EXPECT_EQ(p2p_peer.port(), config.p2p.remote_port);
  EXPECT_EQ(p2p_service.local_port, config.p2p.local_port);

  // check the http service
  ASSERT_TRUE(manifest_->HasService(ServiceIdentifier{ServiceType::HTTP}));

  auto const &http_service = manifest_->GetService(ServiceIdentifier{ServiceType::HTTP});
  auto const &http_peer    = http_service.remote_uri.AsPeer();

  EXPECT_EQ(http_peer.address(), config.http.address);
  EXPECT_EQ(http_peer.port(), config.http.remote_port);
  EXPECT_EQ(http_service.local_port, config.http.local_port);

  // check the names
  for (std::size_t i = 0; i < config.lanes.size(); ++i)
  {
    auto const &lane_config = config.lanes.at(i);

    ServiceIdentifier const lane_identifier{ServiceType::LANE, static_cast<uint16_t>(i)};

    // check the http service
    ASSERT_TRUE(manifest_->HasService(lane_identifier));

    auto const &lane_service = manifest_->GetService(lane_identifier);
    auto const &lane_peer    = lane_service.remote_uri.AsPeer();

    EXPECT_EQ(lane_peer.address(), lane_config.address);
    EXPECT_EQ(lane_peer.port(), lane_config.remote_port);
    EXPECT_EQ(lane_service.local_port, lane_config.local_port);
  }
}

INSTANTIATE_TEST_CASE_P(ParamBased, ManifestTests, testing::ValuesIn(TEST_CASES), );

}  // namespace