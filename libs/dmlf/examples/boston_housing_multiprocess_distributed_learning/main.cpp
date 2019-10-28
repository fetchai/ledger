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
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
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

int main(int argc, char **argv)
{
  // This example will create muddle networking distributed client with simple regression neural net
  // and learns how to predict prices from Boston Housing dataset

  if (argc != 8)
  {
    std::cout << "Args: boston_data.csv boston_label.csv random_seed(int) learning_rate(float) "
                 "results_directory networker_config instance_number"
              << std::endl;
    return 1;
  }

  /**
   * Prepare configuration
   */

  ClientParams<DataType> client_params;

  // Command line parameters
  std::string images_filename = argv[1];
  std::string labels_filename = argv[2];
  SizeType    seed            = strtoul(argv[3], nullptr, 10);
  DataType    learning_rate   = static_cast<DataType>(strtof(argv[4], nullptr));
  std::string results_dir     = argv[5];
  std::string config          = std::string(argv[6]);
  int         instance_number = std::atoi(argv[7]);

  // Distributed learning parameters:
  SizeType number_of_clients  = 6;
  SizeType number_of_rounds   = 200;
  client_params.max_updates   = 16;  // Round ends after this number of batches
  SizeType number_of_peers    = 3;
  client_params.batch_size    = 32;
  client_params.learning_rate = learning_rate;
  float test_set_ratio        = 0.00f;

  /**
   * Prepare environment
   */

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Load data
  TensorType data_tensor = fetch::ml::dataloaders::ReadCSV<TensorType>(images_filename).Transpose();
  TensorType label_tensor =
      fetch::ml::dataloaders::ReadCSV<TensorType>(labels_filename).Transpose();

  // Shuffle data
  utilities::Shuffle(data_tensor, label_tensor, seed);

  // Create learning client
  auto client = fetch::dmlf::distributed_learning::utilities::MakeBostonClient<TensorType>(
      std::to_string(instance_number), client_params, data_tensor, label_tensor, test_set_ratio,
      console_mutex_ptr);

  // Create networker and assign shuffle algorithm
  auto networker = std::make_shared<fetch::dmlf::MuddleLearnerNetworker>(config, instance_number);
  networker->Initialize<fetch::dmlf::Update<TensorType>>();

  networker->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
      networker->GetPeerCount(), number_of_peers));

  // Give client pointer to its networker
  client->SetNetworker(networker);

  // Create loss csv file
  std::string results_filename = results_dir + "/fetch_" + std::to_string(number_of_clients) +
                                 "_Adam_" + std::to_string(float(learning_rate)) + "_" +
                                 std::to_string(seed) + "_FC3.csv";
  std::ofstream lossfile(results_filename, std::ofstream::out);

  if (!lossfile)
  {
    throw fetch::ml::exceptions::InvalidFile("Bad output file");
  }

  /**
   * Main loop
   */

  for (SizeType it{0}; it < number_of_rounds; ++it)
  {
    std::cout << "================= ROUND : " << it << " =================" << std::endl;

    // Start client
    client->Run();

    // Write statistic to csv
    std::cout << it << "\t"
              << static_cast<double>(utilities::Test(client->GetModel(), data_tensor, label_tensor))
              << std::endl;
    lossfile << it << ","
             << static_cast<double>(utilities::Test(client->GetModel(), data_tensor, label_tensor))
             << std::endl;
  }

  std::cout << "Results saved in " << results_filename << std::endl;

  return 0;
}
