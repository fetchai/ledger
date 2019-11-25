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

#include "dmlf/collective_learning/client_algorithm.hpp"
#include "dmlf/collective_learning/utilities/mnist_client_utilities.hpp"
#include "dmlf/collective_learning/utilities/utilities.hpp"
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/tensor.hpp"

#include <algorithm>
#include <iostream>
#include <limits.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::dmlf::collective_learning;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

/**
 * helper function for stripping instance number from hostname
 */
std::uint64_t InstanceFromHostname(std::string &hostname)
{
  std::size_t current  = 0;
  std::size_t previous = 0;
  std::string instance;
  std::string delim{'-'};

  current = hostname.find(delim);
  while (current != std::string::npos)
  {
    previous = current + 1;
    current  = hostname.find(delim, previous);
  }
  instance = (hostname.substr(previous, current - previous));

  return std::stoul(instance);
}

int main(int argc, char **argv)
{
  static constexpr std::size_t HOST_NAME_MAX_LEN = 12;

  // This example will create muddle networking distributed client with simple classification neural
  // net and learns how to predict hand written digits from MNIST dataset

  if (argc != 3)
  {
    std::cout << "learner_config.json networker_config.json" << std::endl;
    return 1;
  }

  // get the host name
  char tmp_hostname[HOST_NAME_MAX_LEN];
  gethostname(tmp_hostname, HOST_NAME_MAX_LEN);
  std::string host_name(tmp_hostname);
  std::cout << "host_name: " << host_name << std::endl;
  std::uint64_t instance_number = InstanceFromHostname(host_name);

  // get the config file
  fetch::json::JSONDocument                                doc;
  fetch::dmlf::collective_learning::ClientParams<DataType> client_params =
      fetch::dmlf::collective_learning::utilities::ClientParamsFromJson<TensorType>(
          std::string(argv[1]), doc);

  auto     data_file      = doc["data"].As<std::string>();
  auto     labels_file    = doc["labels"].As<std::string>();
  auto     n_rounds       = doc["n_rounds"].As<SizeType>();
  auto     n_peers        = doc["n_peers"].As<SizeType>();
  auto     test_set_ratio = doc["test_set_ratio"].As<float>();
  SizeType start_time     = 0;
  if (!doc["start_time"].IsUndefined())
  {
    start_time = doc["start_time"].As<SizeType>();
  }
  SizeType muddle_delay = 30;
  if (!doc["muddle_delay"].IsUndefined())
  {
    muddle_delay = doc["muddle_delay"].As<SizeType>();
  }

  // get the network config file
  fetch::json::JSONDocument network_doc;
  std::ifstream             network_config_file{std::string(argv[2])};
  std::string               text((std::istreambuf_iterator<char>(network_config_file)),
                                 std::istreambuf_iterator<char>());
  network_doc.Parse(text.c_str());

  /**
   * Prepare environment
   */
  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  // Create console mutex
  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Pause until start time
  std::cout << "start_time: " << start_time << std::endl;
  if (start_time != 0) {
    SizeType now = static_cast<SizeType>(std::time(nullptr));
    if (now < start_time) {
      SizeType diff = start_time - now;
      std::cout << "Waiting for " << diff << " seconds delay before starting..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(diff));
    } else {
      std::cout << "Start time is in the past" << std::endl;
    }
  }

  // Create networker and assign shuffle algorithm
  auto networker =
      std::make_shared<fetch::dmlf::MuddleLearnerNetworker>(network_doc, instance_number);
  networker->Initialize<fetch::dmlf::Update<TensorType>>();
  networker->SetShuffleAlgorithm(
      std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(networker->GetPeerCount(), n_peers));

  // Pause to let muddle get set up
  std::this_thread::sleep_for(std::chrono::seconds(muddle_delay));

  // Create learning client
  auto client = fetch::dmlf::collective_learning::utilities::MakeMNISTClient<TensorType>(
      std::to_string(instance_number), client_params, data_file, labels_file, test_set_ratio,
      networker, console_mutex_ptr);

  /**
   * Main loop
   */

  for (SizeType it{0}; it < n_rounds; ++it)
  {
    // Start all clients
    std::cout << "================= ROUND : " << it << " =================" << std::endl;

    client->RunAlgorithms();
  }
  system("gsutil cp /app/results/* gs://ml-3000/results/");
  while (true)
  {
    std::cout << "Sleeping" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));
  }
  return 0;
}
