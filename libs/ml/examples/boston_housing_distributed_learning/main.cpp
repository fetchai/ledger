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

#include "dmlf/local_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/distributed_learning/distributed_learning_client.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

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
    SizeType id, ClientParams<DataType> &client_params, TensorType &data_tensor,
    TensorType &label_tensor, float test_set_ratio, std::shared_ptr<std::mutex> console_mutex_ptr)
{
  // Initialise model
  std::shared_ptr<fetch::ml::Graph<TensorType>> g_ptr =
      std::make_shared<fetch::ml::Graph<TensorType>>();

  client_params.inputs_names = {g_ptr->template AddNode<PlaceHolder<TensorType>>("Input", {})};
  g_ptr->template AddNode<FullyConnected<TensorType>>("FC1", {"Input"}, 13u, 10u);
  g_ptr->template AddNode<Relu<TensorType>>("Relu1", {"FC1"});
  g_ptr->template AddNode<FullyConnected<TensorType>>("FC2", {"Relu1"}, 10u, 10u);
  g_ptr->template AddNode<Relu<TensorType>>("Relu2", {"FC2"});
  g_ptr->template AddNode<FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u, 1u);
  client_params.label_name = g_ptr->template AddNode<PlaceHolder<TensorType>>("Label", {});
  client_params.error_name =
      g_ptr->template AddNode<MeanSquareErrorLoss<TensorType>>("Error", {"FC3", "Label"});
  g_ptr->Compile();

  // Initialise DataLoader
  std::shared_ptr<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>> dataloader_ptr =
      std::make_shared<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>>();
  dataloader_ptr->AddData(data_tensor, label_tensor);

  dataloader_ptr->SetTestRatio(test_set_ratio);
  dataloader_ptr->SetRandomMode(true);
  // Initialise Optimiser
  std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>> optimiser_ptr =
      std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
          std::shared_ptr<fetch::ml::Graph<TensorType>>(g_ptr), client_params.inputs_names,
          client_params.label_name, client_params.error_name, client_params.learning_rate);

  return std::make_shared<TrainingClient<TensorType>>(
      std::to_string(id), g_ptr, dataloader_ptr, optimiser_ptr, client_params, console_mutex_ptr);
}

/**
 * Get loss of given model on given dataset
 * @param g_ptr model
 * @param data_tensor input
 * @param label_tensor label
 * @return
 */
DataType Test(std::shared_ptr<fetch::ml::Graph<TensorType>> const &g_ptr,
              TensorType const &data_tensor, TensorType const &label_tensor)
{
  g_ptr->SetInput("Input", data_tensor);
  g_ptr->SetInput("Label", label_tensor);
  return *(g_ptr->Evaluate("Error").begin());
}

/**
 * Split data to multiple parts
 * @param data input data
 * @param number_of_parts number of parts
 * @return vector of tensors
 */
std::vector<TensorType> Split(TensorType &data, SizeType number_of_parts)
{
  SizeType axis      = data.shape().size() - 1;
  SizeType data_size = data.shape().at(axis);

  // Split data for each client
  std::vector<SizeType> splitting_points;

  SizeType client_data_size         = data_size / number_of_parts;
  SizeType index                    = 0;
  SizeType current_client_data_size = client_data_size;
  for (SizeType i = 0; i < number_of_parts; i++)
  {
    if (i == number_of_parts - 1)
    {
      current_client_data_size = data_size - index;
    }
    splitting_points.push_back(current_client_data_size);
    index += client_data_size;
  }

  return TensorType::Split(data, splitting_points, axis);
}

void Shuffle(TensorType &data, TensorType &labels, SizeType const &seed = 54)
{
  TensorType data_out   = data.Copy();
  TensorType labels_out = labels.Copy();

  std::vector<SizeType> indices;
  SizeType              axis = data.shape().size() - 1;

  for (SizeType i{0}; i < data.shape().at(axis); i++)
  {
    indices.push_back(i);
  }

  fetch::random::LaggedFibonacciGenerator<> lfg(seed);
  fetch::random::Shuffle(lfg, indices, indices);

  for (SizeType i{0}; i < data.shape().at(axis); i++)
  {
    auto data_it       = data.View(i).begin();
    auto data_out_it   = data_out.View(indices.at(i)).begin();
    auto labels_it     = labels.View(i).begin();
    auto labels_out_it = labels_out.View(indices.at(i)).begin();

    while (data_it.is_valid())
    {
      *data_out_it = *data_it;

      ++data_it;
      ++data_out_it;
    }

    while (labels_it.is_valid())
    {
      *labels_out_it = *labels_it;

      ++labels_it;
      ++labels_out_it;
    }
  }

  data   = data_out;
  labels = labels_out;
}

/**
 * Averages weights between all clients
 * @param clients
 */
void SynchroniseWeights(std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients)
{
  VectorTensorType new_weights = clients[0]->GetWeights();

  // Sum all weights
  for (SizeType i{1}; i < clients.size(); ++i)
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
    fetch::math::Divide(new_weights.at(j), static_cast<DataType>(clients.size()),
                        new_weights.at(j));
  }

  // Update models of all clients by average model
  for (auto &c : clients)
  {
    c->SetWeights(new_weights);
  }
}

int main(int argc, char **argv)
{
  if (argc != 6)
  {
    std::cout << "Args: boston_data.csv boston_label.csv random_seed(int) learning_rate(float) "
                 "results_directory"
              << std::endl;
    return 1;
  }

  ClientParams<DataType> client_params;

  bool        synchronise   = false;
  SizeType    seed          = strtoul(argv[3], nullptr, 10);
  DataType    learning_rate = static_cast<DataType>(strtof(argv[4], nullptr));
  std::string results_dir   = argv[5];

  SizeType number_of_clients     = 5;
  SizeType number_of_rounds      = 50;
  client_params.iterations_count = 20;
  client_params.batch_size       = 32;
  client_params.learning_rate    = static_cast<DataType>(.001f);
  float    test_set_ratio        = 0.03f;
  SizeType number_of_peers       = 3;

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Load data
  TensorType data_tensor  = fetch::ml::dataloaders::ReadCSV<TensorType>(argv[1]).Transpose();
  TensorType label_tensor = fetch::ml::dataloaders::ReadCSV<TensorType>(argv[2]).Transpose();

  // Shuffle data
  Shuffle(data_tensor, label_tensor, seed);

  // Split data
  std::vector<TensorType> data_tensors  = Split(data_tensor, number_of_clients);
  std::vector<TensorType> label_tensors = Split(label_tensor, number_of_clients);

  std::cout << "FETCH Distributed BOSTON HOUSING Demo" << std::endl;

  std::vector<std::shared_ptr<fetch::dmlf::LocalLearnerNetworker>> networkers(number_of_clients);

  // Create networkers
  for (SizeType i(0); i < number_of_clients; ++i)
  {
    networkers[i] = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
    networkers[i]->Initialize<fetch::dmlf::Update<TensorType>>();
  }

  for (SizeType i(0); i < number_of_clients; ++i)
  {
    networkers[i]->AddPeers(networkers);
    networkers[i]->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers[i]->GetPeerCount(), number_of_peers));
  }

  std::vector<std::shared_ptr<TrainingClient<TensorType>>> clients(number_of_clients);
  for (SizeType i{0}; i < number_of_clients; ++i)
  {
    // Instantiate NUMBER_OF_CLIENTS clients
    clients[i] = MakeClient(i, client_params, data_tensors.at(i), label_tensors.at(i),
                            test_set_ratio, console_mutex_ptr);
    // TODO(1597): Replace ID with something more sensible
  }

  for (SizeType i{0}; i < number_of_clients; ++i)
  {
    // Give each client pointer to coordinator
    clients[i]->SetNetworker(networkers[i]);
  }

  std::string results_filename = results_dir + "/fetch_" + std::to_string(number_of_clients) +
                                 "_Adam_" + std::to_string(float(learning_rate)) + "_" +
                                 std::to_string(seed) + "_FC3.csv";
  std::ofstream lossfile(results_filename, std::ofstream::out);

  if (!lossfile)
  {
    throw std::runtime_error("Bad output file");
  }

  // Main loop
  for (SizeType it{0}; it < number_of_rounds; ++it)
  {
    // Start all clients
    std::cout << "ROUND : " << it << "\t";
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

    std::cout << it;
    for (auto &c : clients)
    {
      std::cout << "\t" << static_cast<double>(Test(c->GetModel(), data_tensor, label_tensor));
    }
    std::cout << std::endl;

    // Synchronize weights by giving all clients average of all client's weights
    if (synchronise)
    {
      lossfile << it;
    }
    for (auto &c : clients)
    {
      lossfile << "," << static_cast<double>(Test(c->GetModel(), data_tensor, label_tensor));
    }
    lossfile << std::endl;

    if (synchronise)
    {
      SynchroniseWeights(clients);
    }
  }

  std::cout << "Results saved in " << results_filename << std::endl;

  return 0;
}
