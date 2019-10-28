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

#include "dmlf/distributed_learning/distributed_learning_client.hpp"
#include "dmlf/distributed_learning/utilities/boston_housing_client_utilities.hpp"
#include "dmlf/distributed_learning/utilities/distributed_learning_utilities.hpp"
#include "dmlf/networkers/local_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::dmlf::distributed_learning;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

int main(int argc, char **argv)
{
  // This example will create multiple local distributed clients with simple regression neural net
  // and learns how to predict prices from Boston Housing dataset
  // read input config file
  if (argc != 2)
  {
    std::cout << "config_file.json" << std::endl;
    return 1;
  }

  /**
   * Prepare configuration
   */
  fetch::json::JSONDocument doc;
  std::ifstream             config_file(argv[1]);
  std::string text((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
  doc.Parse(text.c_str());

  ClientParams<DataType> client_params;
  auto                   data_file   = doc["data"].As<std::string>();
  auto                   labels_file = doc["labels"].As<std::string>();
  auto                   results_dir = doc["results"].As<std::string>();
  auto                   n_clients   = doc["n_clients"].As<SizeType>();
  auto                   n_peers     = doc["n_peers"].As<SizeType>();
  auto                   n_rounds    = doc["n_rounds"].As<SizeType>();
  auto                   synchronise = doc["synchronise"].As<bool>();
  client_params.max_updates          = doc["max_updates"].As<SizeType>();
  client_params.batch_size           = doc["batch_size"].As<SizeType>();
  client_params.learning_rate        = static_cast<DataType>(doc["learning_rate"].As<float>());
  auto seed                          = doc["random_seed"].As<SizeType>();
  auto test_set_ratio                = doc["test_set_ratio"].As<float>();

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Load data
  TensorType data_tensor  = fetch::ml::dataloaders::ReadCSV<TensorType>(data_file).Transpose();
  TensorType label_tensor = fetch::ml::dataloaders::ReadCSV<TensorType>(labels_file).Transpose();

  // Shuffle data
  utilities::Shuffle(data_tensor, label_tensor, seed);

  // Split data
  std::vector<TensorType> data_tensors =
      fetch::dmlf::distributed_learning::utilities::Split(data_tensor, n_clients);
  std::vector<TensorType> label_tensors =
      fetch::dmlf::distributed_learning::utilities::Split(label_tensor, n_clients);

  // Create networkers
  std::vector<std::shared_ptr<fetch::dmlf::LocalLearnerNetworker>> networkers(n_clients);
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers[i] = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
    networkers[i]->Initialize<fetch::dmlf::Update<TensorType>>();
  }

  // Add peers to networkers and initialise shuffle algorithm
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers[i]->AddPeers(networkers);
    networkers[i]->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers[i]->GetPeerCount(), n_peers));
  }

  // Create training clients
  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(n_clients);
  for (SizeType i{0}; i < n_clients; ++i)
  {
    // Instantiate n_clients clients
    clients[i] = fetch::dmlf::distributed_learning::utilities::MakeBostonClient(
        std::to_string(i), client_params, data_tensors.at(i), label_tensors.at(i), test_set_ratio,
        console_mutex_ptr);
  }

  for (SizeType i{0}; i < n_clients; ++i)
  {
    // Give each client pointer to its networker
    clients[i]->SetNetworker(networkers[i]);
  }

  // Create loss csv file
  std::string results_filename = results_dir + "/fetch_" + std::to_string(n_clients) + "_Adam_" +
                                 std::to_string(float(client_params.learning_rate)) + "_" +
                                 std::to_string(seed) + "_FC3.csv";
  std::ofstream lossfile(results_filename, std::ofstream::out);

  if (!lossfile)
  {
    throw fetch::ml::exceptions::InvalidFile("Bad output file");
  }

  /**
   * Main loop
   */

  for (SizeType it{0}; it < n_rounds; ++it)
  {
    // Start all clients
    std::cout << "================= ROUND : " << it << " =================" << std::endl;
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      threads.emplace_back([&c] { c->Run(); });
    }

    // Wait for everyone to finish
    for (auto &t : threads)
    {
      t.join();
    }

    // Write statistic to csv
    std::cout << it;
    lossfile << it;
    for (auto &c : clients)
    {
      std::cout << "\t"
                << static_cast<double>(utilities::Test(c->GetModel(), data_tensor, label_tensor));
      lossfile << ","
               << static_cast<double>(utilities::Test(c->GetModel(), data_tensor, label_tensor));
    }
    std::cout << std::endl;
    lossfile << std::endl;

    // Synchronize weights by giving all clients average of all client's weights
    if (synchronise)
    {
      std::cout << std::endl << "Synchronising weights" << std::endl;
      fetch::dmlf::distributed_learning::utilities::SynchroniseWeights<TensorType>(clients);
    }
  }

  std::cout << "Results saved in " << results_filename << std::endl;

  return 0;
}
