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

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

std::shared_ptr<TrainingClient<TensorType>> MakeClient(std::string const &     id,
                                                       ClientParams<DataType> &client_params,
                                                       std::string const &     images,
                                                       std::string const &     labels,
                                                       float                   test_set_ratio)
{
  // Initialise model
  std::shared_ptr<fetch::ml::Graph<TensorType>> g_ptr =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  client_params.inputs_names = {g_ptr->template AddNode<PlaceHolder<TensorType>>("Input", {})};
  g_ptr->template AddNode<FullyConnected<TensorType>>("FC1", {"Input"}, 28u * 28u, 10u);
  g_ptr->template AddNode<Relu<TensorType>>("Relu1", {"FC1"});
  g_ptr->template AddNode<FullyConnected<TensorType>>("FC2", {"Relu1"}, 10u, 10u);
  g_ptr->template AddNode<Relu<TensorType>>("Relu2", {"FC2"});
  g_ptr->template AddNode<FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u, 10u);
  g_ptr->template AddNode<Softmax<TensorType>>("Softmax", {"FC3"});
  client_params.label_name = g_ptr->template AddNode<PlaceHolder<TensorType>>("Label", {});
  client_params.error_name =
      g_ptr->template AddNode<CrossEntropyLoss<TensorType>>("Error", {"Softmax", "Label"});

  // Initialise DataLoader
  std::shared_ptr<fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>> dataloader_ptr =
      std::make_shared<fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>>(images, labels);
  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);

  // Initialise Optimiser
  std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>> optimiser_ptr =
      std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
          std::shared_ptr<fetch::ml::Graph<TensorType>>(g_ptr), client_params.inputs_names,
          client_params.label_name, client_params.error_name, client_params.learning_rate);

  return std::make_shared<TrainingClient<TensorType>>(id, g_ptr, dataloader_ptr, optimiser_ptr,
                                                      client_params);
}

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  CoordinatorParams      coord_params;
  ClientParams<DataType> client_params;

  SizeType number_of_clients    = 10;
  SizeType number_of_rounds     = 10;
  coord_params.mode             = CoordinatorMode::SEMI_SYNCHRONOUS;
  coord_params.iterations_count = 100;
  client_params.batch_size      = 32;
  client_params.learning_rate   = static_cast<DataType>(.001f);
  float test_set_ratio          = 0.03f;
  client_params.number_of_peers = 3;

  std::shared_ptr<Coordinator> coordinator = std::make_shared<Coordinator>(coord_params);

  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(number_of_clients);
  for (SizeType i{0}; i < number_of_clients; ++i)
  {
    // Instantiate NUMBER_OF_CLIENTS clients
    clients[i] = MakeClient(std::to_string(i), client_params, av[1], av[2], test_set_ratio);
    // TODO(1597): Replace ID with something more sensible
  }

  for (SizeType i{0}; i < number_of_clients; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);

    // Give each client pointer to coordinator
    clients[i]->SetCoordinator(coordinator);
  }

  // Main loop
  for (SizeType it{0}; it < number_of_rounds; ++it)
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
    for (SizeType i{1}; i < number_of_clients; ++i)
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
      fetch::math::Divide(new_weights.at(j), static_cast<DataType>(number_of_clients),
                          new_weights.at(j));
    }

    // Update models of all clients by average model
    for (unsigned int i(0); i < number_of_clients; ++i)
    {
      clients[i]->SetWeights(new_weights);
    }
  }

  return 0;
}
