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
#include "dmlf/distributed_learning/utilities/distributed_learning_utilities.hpp"
#include "dmlf/distributed_learning/utilities/mnist_client_utilities.hpp"
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

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

int main(int ac, char **av)
{
  // This example will create muddle networking distributed client with simple classification neural
  // net and learns how to predict hand written digits from MNIST dataset

  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte "
                 "networker_config instance_number"
              << std::endl;
    return 1;
  }

  /**
   * Prepare configuration
   */

  ClientParams<DataType> client_params;

  // Command line parameters
  std::string images_filename = av[1];
  std::string labels_filename = av[2];
  std::string config          = std::string(av[3]);
  int         instance_number = std::atoi(av[4]);

  // Distributed learning parameters:
  SizeType number_of_rounds   = 10;
  client_params.max_updates   = 100;  // Round ends after this number of batches
  SizeType number_of_peers    = 3;
  client_params.batch_size    = 32;
  client_params.learning_rate = static_cast<DataType>(.001f);
  float test_set_ratio        = 0.03f;

  /**
   * Prepare environment
   */
  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  // Create console mutex
  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Create learning client
  auto client = fetch::dmlf::distributed_learning::utilities::MakeMNISTClient<TensorType>(
      std::to_string(instance_number), client_params, images_filename, labels_filename,
      test_set_ratio, console_mutex_ptr);

  // Create networker and assign shuffle algorithm
  auto networker = std::make_shared<fetch::dmlf::MuddleLearnerNetworker>(config, instance_number);
  networker->Initialize<fetch::dmlf::Update<TensorType>>();

  networker->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
      networker->GetPeerCount(), number_of_peers));

  // Give client pointer to its networker
  client->SetNetworker(networker);

  /**
   * Main loop
   */

  for (SizeType it{0}; it < number_of_rounds; ++it)
  {
    // Start all clients
    std::cout << "================= ROUND : " << it << " =================" << std::endl;

    client->Run();
  }

  return 0;
}
