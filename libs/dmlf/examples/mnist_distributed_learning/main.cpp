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
#include "dmlf/networkers/local_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/ops/activation.hpp"
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
using namespace fetch::ml::distributed_learning;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

std::shared_ptr<TrainingClient<TensorType>> MakeClient(
    std::string const &id, ClientParams<DataType> &client_params, std::string const &images,
    std::string const &labels, float test_set_ratio, std::shared_ptr<std::mutex> console_mutex_ptr)
{
  // Initialise model
  auto model_ptr = std::make_shared<fetch::ml::model::Sequential<TensorType>>();

  model_ptr->template Add<FullyConnected<TensorType>>(28u * 28u, 10u,
                                                      fetch::ml::details::ActivationType::RELU);
  model_ptr->template Add<FullyConnected<TensorType>>(10u, 10u,
                                                      fetch::ml::details::ActivationType::RELU);
  model_ptr->template Add<FullyConnected<TensorType>>(10u, 10u,
                                                      fetch::ml::details::ActivationType::SOFTMAX);

  client_params.inputs_names = {model_ptr->InputName()};
  client_params.label_name   = model_ptr->LabelName();
  client_params.error_name   = model_ptr->ErrorName();

  // Initialise DataLoader
  auto dataloader_ptr =
      std::make_unique<fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>>(images, labels);
  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);

  model_ptr->SetDataloader(std::move(dataloader_ptr));
  model_ptr->Compile(fetch::ml::OptimiserType::ADAM, fetch::ml::ops::LossType::CROSS_ENTROPY);

  return std::make_shared<TrainingClient<TensorType>>(id, model_ptr, client_params,
                                                      console_mutex_ptr);
}

int main(int ac, char **av)
{
  if (ac != 2)
  {
    std::cout << "Usage : " << av[0] << "config_file.json" << std::endl;
    return 1;
  }
  fetch::json::JSONDocument doc;
  std::ifstream             config_file(av[1]);
  std::string text((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
  doc.Parse(text.c_str());

  ClientParams<DataType> client_params;
  auto                   data_file   = doc["data"].As<std::string>();
  auto                   labels_file = doc["labels"].As<std::string>();
  //  auto                   results_dir = doc["results"].As<std::string>();
  auto n_clients              = doc["n_clients"].As<SizeType>();
  auto n_peers                = doc["n_peers"].As<SizeType>();
  auto n_rounds               = doc["n_rounds"].As<SizeType>();
  auto synchronise            = doc["synchronise"].As<bool>();
  client_params.max_updates   = doc["max_updates"].As<SizeType>();
  client_params.batch_size    = doc["batch_size"].As<SizeType>();
  client_params.learning_rate = static_cast<DataType>(doc["learning_rate"].As<float>());
  //  auto seed                          = doc["random_seed"].As<SizeType>();
  auto test_set_ratio = doc["test_set_ratio"].As<float>();

  //  SizeType n_clients                    = 10;
  //  SizeType n_rounds                     = 10;
  //  bool     synchronise                      = false;
  //  client_params.max_updates                     = 100;
  //  SizeType n_peers                      = 3;
  //  client_params.batch_size                      = 32;
  //  client_params.learning_rate                   = static_cast<DataType>(.001f);
  //  float                       test_set_ratio    = 0.03f;
  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  std::vector<std::shared_ptr<fetch::dmlf::LocalLearnerNetworker>> networkers(n_clients);

  std::cout << "FETCH Distributed MNIST Demo" << std::endl;

  // Create networkers
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers[i] = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
    networkers[i]->Initialize<fetch::dmlf::Update<TensorType>>();
  }

  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers[i]->AddPeers(networkers);
    networkers[i]->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers[i]->GetPeerCount(), n_peers));
  }

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(n_clients);
  for (SizeType i{0}; i < n_clients; ++i)
  {
    // Instantiate n_clients clients
    clients[i] = MakeClient(std::to_string(i), client_params, av[1], av[2], test_set_ratio,
                            console_mutex_ptr);
  }

  for (SizeType i{0}; i < n_clients; ++i)
  {
    // Give each client pointer to coordinator
    clients[i]->SetNetworker(networkers[i]);
  }

  // Main loop
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

    if (!synchronise)
    {
      continue;
    }

    // Synchronize weights by giving all clients average of all client's weights
    VectorTensorType new_weights = clients[0]->GetWeights();

    // Sum all weights
    for (SizeType i{1}; i < n_clients; ++i)
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
      fetch::math::Divide(new_weights.at(j), static_cast<DataType>(n_clients), new_weights.at(j));
    }

    // Update models of all clients by average model
    for (uint32_t i(0); i < n_clients; ++i)
    {
      clients[i]->SetWeights(new_weights);
    }
  }

  return 0;
}
