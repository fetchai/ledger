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

#include "dmlf/collective_learning/client_algorithm.hpp"
#include "dmlf/collective_learning/utilities/mnist_client_utilities.hpp"
#include "dmlf/collective_learning/utilities/utilities.hpp"
#include "dmlf/deprecated/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/tensor/tensor.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::dmlf::collective_learning;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

int main(int argc, char **argv)
{
  // This example will create muddle networking distributed client with simple classification neural
  // net and learns how to predict hand written digits from MNIST dataset

  if (argc != 4)
  {
    std::cout << "learner_config.json networker_config instance_number" << std::endl;
    return 1;
  }

  auto networker_config = std::string(argv[2]);
  int  instance_number  = std::atoi(argv[3]);

  fetch::json::JSONDocument                                doc;
  fetch::dmlf::collective_learning::ClientParams<DataType> client_params =
      fetch::dmlf::collective_learning::utilities::ClientParamsFromJson<TensorType>(
          std::string(argv[1]), doc);

  auto data_file      = doc["data"].As<std::string>();
  auto labels_file    = doc["labels"].As<std::string>();
  auto n_rounds       = doc["n_rounds"].As<SizeType>();
  auto n_peers        = doc["n_peers"].As<SizeType>();
  auto test_set_ratio = doc["test_set_ratio"].As<float>();

  // get the network config file
  fetch::json::JSONDocument network_doc;
  std::ifstream             network_config_file{networker_config};
  std::string               text((std::istreambuf_iterator<char>(network_config_file)),
                                 std::istreambuf_iterator<char>());
  network_doc.Parse(text.c_str());

  /**
   * Prepare environment
   */
  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  // Create console mutex
  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Create networker and assign shuffle algorithm
  auto networker = std::make_shared<fetch::dmlf::deprecated_MuddleLearnerNetworker>(
      network_doc, instance_number);
  networker->Initialize<fetch::dmlf::deprecated_Update<TensorType>>();
  networker->SetShuffleAlgorithm(
      std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(networker->GetPeerCount(), n_peers));

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

  return 0;
}
