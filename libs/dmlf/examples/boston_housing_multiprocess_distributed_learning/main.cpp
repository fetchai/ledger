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
#include "dmlf/distributed_learning/utilities/utilities.hpp"
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
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

  if (argc != 4)
  {
    std::cout << "learner_config.json networker_config instance_number" << std::endl;
    return 1;
  }

  auto networker_config = std::string(argv[2]);
  int  instance_number  = std::atoi(argv[3]);

  fetch::json::JSONDocument                                 doc;
  fetch::dmlf::distributed_learning::ClientParams<DataType> client_params =
      fetch::dmlf::distributed_learning::utilities::ClientParamsFromJson<TensorType>(
          std::string(argv[1]), doc);

  auto data_file      = doc["data"].As<std::string>();
  auto labels_file    = doc["labels"].As<std::string>();
  auto results_dir    = doc["results"].As<std::string>();
  auto n_peers        = doc["n_peers"].As<SizeType>();
  auto n_rounds       = doc["n_rounds"].As<SizeType>();
  auto seed           = doc["random_seed"].As<SizeType>();
  auto test_set_ratio = doc["test_set_ratio"].As<float>();

  /**
   * Prepare environment
   */

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();

  // Load data

  TensorType data_tensor  = fetch::ml::dataloaders::ReadCSV<TensorType>(data_file).Transpose();
  TensorType label_tensor = fetch::ml::dataloaders::ReadCSV<TensorType>(labels_file).Transpose();

  // Shuffle data
  utilities::Shuffle(data_tensor, label_tensor, seed);

  // Create learning client
  auto client = fetch::dmlf::distributed_learning::utilities::MakeBostonClient<TensorType>(
      std::to_string(instance_number), client_params, data_tensor, label_tensor, test_set_ratio,
      console_mutex_ptr);

  // Create networker and assign shuffle algorithm

  auto networker =
      std::make_shared<fetch::dmlf::MuddleLearnerNetworker>(networker_config, instance_number);
  networker->Initialize<fetch::dmlf::Update<TensorType>>();

  networker->SetShuffleAlgorithm(
      std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(networker->GetPeerCount(), n_peers));

  // Give client pointer to its networker
  client->SetNetworker(networker);

  /**
   * Main loop
   */

  for (SizeType it{0}; it < n_rounds; ++it)
  {
    std::cout << "================= ROUND : " << it << " =================" << std::endl;

    // Start client
    client->Run();
  }

  return 0;
}
