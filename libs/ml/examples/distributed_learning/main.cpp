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

#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/distributed_learning/coordinator.hpp"
#include "ml/distributed_learning/distributed_learning_client.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "mnist_client.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#define NUMBER_OF_CLIENTS 10
#define NUMBER_OF_ITERATIONS 100
#define NUMBER_OF_ROUNDS 10
#define SYNCHRONIZATION_MODE CoordinatorMode::SEMI_SYNCHRONOUS

#define BATCH_SIZE 32
#define LEARNING_RATE .001f
#define TEST_SET_RATIO 0.03f
#define NUMBER_OF_PEERS 3

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::shared_ptr<Coordinator> coordinator =
      std::make_shared<Coordinator>(SYNCHRONIZATION_MODE, NUMBER_OF_ITERATIONS);

  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(NUMBER_OF_CLIENTS);
  for (SizeType i{0}; i < NUMBER_OF_CLIENTS; ++i)
  {
    // Instantiate NUMBER_OF_CLIENTS clients
    clients[i] = std::make_shared<MNISTClient<TensorType>>(
        av[1], av[2], std::to_string(i), BATCH_SIZE, static_cast<DataType>(LEARNING_RATE),
        TEST_SET_RATIO, NUMBER_OF_PEERS);
    // TODO(1597): Replace ID with something more sensible
  }

  for (SizeType i{0}; i < NUMBER_OF_CLIENTS; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);

    // Give each client pointer to coordinator
    clients[i]->SetCoordinator(coordinator);
  }

  // Main loop
  for (SizeType it{0}; it < NUMBER_OF_ROUNDS; ++it)
  {
    // Start all clients
    coordinator->Reset();
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

    if (coordinator->GetMode() == CoordinatorMode::ASYNCHRONOUS)
    {
      continue;
    }

    // Synchronize weights by giving all clients average of all client's weights
    VectorTensorType new_weights = clients[0]->GetWeights();

    // Sum all wights
    for (SizeType i{1}; i < NUMBER_OF_CLIENTS; ++i)
    {
      VectorTensorType other_weights = clients[i]->GetWeights();

      for (SizeType j{0}; j < other_weights.size(); j++)
      {
        fetch::math::Add(new_weights.at(j), other_weights.at(j), new_weights.at(j));
      }
    }

    // Divide weights by number of clients to calculate the average
    for (SizeType j{0}; j < new_weights.size(); j++)
    {
      fetch::math::Divide(new_weights.at(j), static_cast<DataType>(NUMBER_OF_CLIENTS),
                          new_weights.at(j));
    }

    // Update models of all clients by average model
    for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
    {
      clients[i]->SetWeights(new_weights);
    }
  }

  return 0;
}
