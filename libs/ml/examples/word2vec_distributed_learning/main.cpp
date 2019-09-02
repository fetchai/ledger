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

#include "core/random.hpp"
#include "file_loader.hpp"
#include "math/clustering/knn.hpp"
#include "math/matrix_operations.hpp"
#include "math/statistics/mean.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/distributed_learning/coordinator.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "model_saver.hpp"
#include "word2vec_client.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#define NUMBER_OF_CLIENTS 3
#define NUMBER_OF_ITERATIONS 100
#define NUMBER_OF_ROUNDS 10
#define SYNCHRONIZATION_MODE CoordinatorMode::ASYNCHRONOUS

#define BATCH_SIZE 128
#define LEARNING_RATE .001f
#define TEST_SET_RATIO 0.03f
#define NUMBER_OF_PEERS 2

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = typename TensorType::SizeType;

int main(int ac, char **av)
{
  if (ac < 2)
  {
    std::cout << "Usage : " << av[0] << " PATH/TO/text8" << std::endl;
    return 1;
  }

  std::shared_ptr<std::mutex> console_mutex_ptr_ = std::make_shared<std::mutex>();

  std::string train_file = av[1];

  std::shared_ptr<Coordinator> coordinator =
      std::make_shared<Coordinator>(SYNCHRONIZATION_MODE, NUMBER_OF_ITERATIONS);

  std::cout << "FETCH Distributed Word2vec Demo -- Asynchronous" << std::endl;

  TrainingParams<TensorType> tp;

  // calc the true starting learning rate
  tp.starting_learning_rate =
      static_cast<DataType>(tp.batch_size) * tp.starting_learning_rate_per_sample;
  tp.ending_learning_rate =
      static_cast<DataType>(tp.batch_size) * tp.ending_learning_rate_per_sample;
  tp.learning_rate_param.starting_learning_rate = tp.starting_learning_rate;
  tp.learning_rate_param.ending_learning_rate   = tp.ending_learning_rate;

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(NUMBER_OF_CLIENTS);
  for (SizeType i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Instantiate NUMBER_OF_CLIENTS clients
    clients[i] = std::make_shared<Word2VecClient<TensorType>>(
        std::to_string(i), tp, train_file, BATCH_SIZE, NUMBER_OF_PEERS, console_mutex_ptr_);
    // TODO(1597): Replace ID with something more sensible
  }

  for (SizeType i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);

    // Give each client pointer to coordinator
    clients[i]->SetCoordinator(coordinator);
  }

  // Main loop
  for (SizeType it(0); it < NUMBER_OF_ROUNDS; ++it)
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
      break;
    }

    // Synchronize weights by giving all clients average of all client's weights
    VectorTensorType new_weights = clients[0]->GetWeights();

    // Sum all weights
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
    for (SizeType i(0); i < NUMBER_OF_CLIENTS; ++i)
    {
      clients[i]->SetWeights(new_weights);
    }
  }

  return 0;
}
