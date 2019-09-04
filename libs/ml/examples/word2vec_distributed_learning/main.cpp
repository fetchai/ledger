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
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/distributed_learning/coordinator.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "word2vec_client.hpp"

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::ml::distributed_learning;

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

  CoordinatorParams           coord_params;
  W2VTrainingParams<DataType> client_params;

  // Distributed learning parameters:
  SizeType number_of_clients    = 10;
  SizeType number_of_rounds     = 10;
  coord_params.mode             = CoordinatorMode::SEMI_SYNCHRONOUS;
  coord_params.iterations_count = 100;
  client_params.batch_size      = 32;
  client_params.learning_rate   = static_cast<DataType>(.001f);
  client_params.number_of_peers = 3;

  // Word2Vec parameters:
  client_params.vocab_file           = av[1];
  client_params.negative_sample_size = 5;  // number of negative sample per word-context pair
  client_params.window_size          = 5;  // window size for context sampling
  client_params.freq_thresh          = DataType{1e-3};  // frequency threshold for subsampling
  client_params.min_count            = 5;               // infrequent word removal threshold
  client_params.embedding_size       = 100;             // dimension of embedding vec

  client_params.k     = 20;       // how many nearest neighbours to compare against
  client_params.word0 = "three";  // test word to consider
  client_params.word1 = "king";
  client_params.word2 = "queen";
  client_params.word3 = "father";

  // calc the true starting learning rate
  client_params.starting_learning_rate = static_cast<DataType>(client_params.batch_size) *
                                         client_params.starting_learning_rate_per_sample;
  client_params.ending_learning_rate = static_cast<DataType>(client_params.batch_size) *
                                       client_params.ending_learning_rate_per_sample;
  client_params.learning_rate_param.starting_learning_rate = client_params.starting_learning_rate;
  client_params.learning_rate_param.ending_learning_rate   = client_params.ending_learning_rate;

  std::shared_ptr<std::mutex>  console_mutex_ptr_ = std::make_shared<std::mutex>();
  std::shared_ptr<Coordinator> coordinator        = std::make_shared<Coordinator>(coord_params);
  std::cout << "FETCH Distributed Word2vec Demo -- Asynchronous" << std::endl;

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(number_of_clients);
  for (SizeType i(0); i < number_of_clients; ++i)
  {
    // Instantiate NUMBER_OF_CLIENTS clients
    clients[i] = std::make_shared<Word2VecClient<TensorType>>(std::to_string(i), client_params,
                                                              console_mutex_ptr_);
    // TODO(1597): Replace ID with something more sensible
  }

  for (SizeType i(0); i < number_of_clients; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);

    // Give each client pointer to coordinator
    clients[i]->SetCoordinator(coordinator);
  }

  // Main loop
  for (SizeType it(0); it < number_of_rounds; ++it)
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

    // Sum all weights
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
    for (SizeType i(0); i < number_of_clients; ++i)
    {
      clients[i]->SetWeights(new_weights);
    }
  }

  return 0;
}
