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
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
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
    std::string id, ClientParams<DataType> &client_params, TensorType &data_tensor,
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

  return std::make_shared<TrainingClient<TensorType>>(id, g_ptr, dataloader_ptr, optimiser_ptr,
                                                      client_params, console_mutex_ptr);
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

int main(int argc, char **argv)
{
  if (argc != 8)
  {
    std::cout << "Args: boston_data.csv boston_label.csv random_seed(int) learning_rate(float) "
                 "results_directory networker_config instance_number"
              << std::endl;
    return 1;
  }

  SizeType    seed            = strtoul(argv[3], nullptr, 10);
  DataType    learning_rate   = static_cast<DataType>(strtof(argv[4], nullptr));
  std::string results_dir     = argv[5];
  std::string config          = std::string(argv[6]);
  int         instance_number = std::atoi(argv[7]);

  ClientParams<DataType> client_params;

  SizeType number_of_clients                    = 6;
  SizeType number_of_rounds                     = 200;
  client_params.max_updates                     = 16;  // Round ends after this number of batches
  SizeType number_of_peers                      = 3;
  client_params.batch_size                      = 32;
  client_params.learning_rate                   = learning_rate;
  float                       test_set_ratio    = 0.00f;
  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Load data
  TensorType data_tensor  = fetch::ml::dataloaders::ReadCSV<TensorType>(argv[1]).Transpose();
  TensorType label_tensor = fetch::ml::dataloaders::ReadCSV<TensorType>(argv[2]).Transpose();

  // Shuffle data
  Shuffle(data_tensor, label_tensor, seed);

  auto client = MakeClient(std::to_string(instance_number), client_params, data_tensor,
                           label_tensor, test_set_ratio, console_mutex_ptr);

  auto networker = std::make_shared<fetch::dmlf::MuddleLearnerNetworker>(config, instance_number);
  networker->Initialize<fetch::dmlf::Update<TensorType>>();

  networker->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
      networker->GetPeerCount(), number_of_peers));

  client->SetNetworker(networker);

  std::string results_filename = results_dir + "/fetch_" + std::to_string(number_of_clients) +
                                 "_Adam_" + std::to_string(float(learning_rate)) + "_" +
                                 std::to_string(seed) + "_FC3.csv";
  std::ofstream lossfile(results_filename, std::ofstream::out);

  if (!lossfile)
  {
    throw fetch::ml::exceptions::InvalidFile("Bad output file");
  }

  // Main loop
  for (SizeType it{0}; it < number_of_rounds; ++it)
  {
    client->Run();

    std::cout << it << "\t"
              << static_cast<double>(Test(client->GetModel(), data_tensor, label_tensor))
              << std::endl;
    lossfile << it << ","
             << static_cast<double>(Test(client->GetModel(), data_tensor, label_tensor))
             << std::endl;
  }

  std::cout << "Results saved in " << results_filename << std::endl;

  return 0;
}
