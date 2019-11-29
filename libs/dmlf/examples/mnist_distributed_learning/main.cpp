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

#include "dmlf/collective_learning/collective_learning_client.hpp"
#include "dmlf/collective_learning/utilities/mnist_client_utilities.hpp"
#include "dmlf/collective_learning/utilities/utilities.hpp"
#include "dmlf/deprecated/local_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/tensor.hpp"

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

/*  Example JSON configuration file:
{
        "data": "datasets/mnist_federated/mnist_images_",
        "labels": "datasets/mnist_federated/mnist_labels_",
        "n_clients": 5,
        "n_peers": 3,
        "n_rounds": 10,
        "synchronise": true,
        "test_set_ratio": 0.1,
        "results": "/tmp/results/",
        "batch_size": 32,
        "max_updates": 100,
        "max_epochs": 20,
        "learning_rate": 0.02,
        "print_loss": false,
}
 */

int main(int argc, char **argv)
{
  // This example will create multiple local distributed clients with simple classification neural
  // net and learns how to predict hand written digits from MNIST dataset

  if (argc != 2)
  {
    std::cout << "Usage : " << argv[0] << " config_file.json" << std::endl;
    return 1;
  }

  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  // handle config params
  fetch::json::JSONDocument                                doc;
  fetch::dmlf::collective_learning::ClientParams<DataType> client_params =
      fetch::dmlf::collective_learning::utilities::ClientParamsFromJson<TensorType>(
          std::string(argv[1]), doc);
  auto data_file      = doc["data"].As<std::string>();
  auto labels_file    = doc["labels"].As<std::string>();
  auto n_clients      = doc["n_clients"].As<SizeType>();
  auto n_peers        = doc["n_peers"].As<SizeType>();
  auto n_rounds       = doc["n_rounds"].As<SizeType>();
  auto synchronise    = doc["synchronise"].As<bool>();
  auto test_set_ratio = doc["test_set_ratio"].As<float>();

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Set up networkers
  std::vector<std::shared_ptr<fetch::dmlf::deprecated_LocalLearnerNetworker>> networkers(n_clients);
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers.at(i) = std::make_shared<fetch::dmlf::deprecated_LocalLearnerNetworker>();
    networkers.at(i)->Initialize<fetch::dmlf::deprecated_Update<TensorType>>();
  }
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers.at(i)->AddPeers(networkers);
    networkers.at(i)->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers.at(i)->GetPeerCount(), n_peers));
  }

  // Create training clients
  std::vector<std::shared_ptr<CollectiveLearningClient<TensorType>>> clients(n_clients);
  for (SizeType i{0}; i < n_clients; ++i)
  {
    clients.at(i) = fetch::dmlf::collective_learning::utilities::MakeMNISTClient<TensorType>(
        std::to_string(i), client_params, data_file + std::to_string(i) + ".csv",
        labels_file + std::to_string(i) + ".csv", test_set_ratio, networkers.at(i),
        console_mutex_ptr);
  }

  /**
   * Main loop
   */

  for (SizeType it{0}; it < n_rounds; ++it)
  {
    // Start all clients
    std::cout << "================= ROUND : " << it << " =================" << std::endl;
    std::vector<std::thread> threads;
    for (auto &c : clients)
    {
      c->RunAlgorithms(threads);
    }

    // Wait for everyone to finish
    for (auto &t : threads)
    {
      t.join();
    }

    // Synchronize weights by giving all clients average of all client's weights
    if (synchronise)
    {
      std::cout << std::endl << "Synchronising weights" << std::endl;
      fetch::dmlf::collective_learning::utilities::SynchroniseWeights<TensorType>(clients);
    }
  }

  return 0;
}
