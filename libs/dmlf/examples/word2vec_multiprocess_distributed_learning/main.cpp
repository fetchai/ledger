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

#include "dmlf/distributed_learning/utilities/utilities.hpp"
#include "dmlf/distributed_learning/word2vec_client.hpp"
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/tensor.hpp"

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::dmlf::distributed_learning;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = typename TensorType::SizeType;

int main(int argc, char **argv)
{
  // This example will create muddle networking distributed client with CBOW Word2Vec model and
  // trains word embeddings based on input text file

  if (argc != 4)
  {
    std::cout << "learner_config.json networker_config instance_number" << std::endl;
    return 1;
  }

  /**
   * Prepare configuration
   */

  fetch::json::JSONDocument doc;
  ClientParams<DataType>    client_params =
      fetch::dmlf::distributed_learning::utilities::ClientParamsFromJson<TensorType>(
          std::string(argv[1]), doc);
  auto word2vec_client_params = std::make_shared<Word2VecTrainingParams<DataType>>(client_params);

  auto networker_config = std::string(argv[2]);
  int  instance_number  = std::atoi(argv[3]);

  auto data_file                              = doc["data"].As<std::string>();
  word2vec_client_params->analogies_test_file = doc["analogies_test_file"].As<std::string>();
  word2vec_client_params->vocab_file          = doc["vocab_file"].As<std::string>();
  word2vec_client_params->test_frequency      = doc["test_frequency"].As<SizeType>();

  // Distributed learning parameters:
  auto        n_peers         = doc["n_peers"].As<SizeType>();
  auto        n_rounds        = doc["n_rounds"].As<SizeType>();
  std::string output_csv_file = doc["results"].As<std::string>();

  /**
   * Prepare environment
   */

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();
  std::cout << "FETCH Distributed Word2vec Demo" << std::endl;

  std::string client_data = fetch::ml::utilities::ReadFile(data_file);

  Word2VecTrainingParams<DataType> cp(*word2vec_client_params);
  cp.data = {client_data};

  // Create learning client
  auto client = std::make_shared<Word2VecClient<TensorType>>(std::to_string(instance_number), cp,
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

  for (SizeType it(0); it < n_rounds; ++it)
  {
    std::cout << "================= ROUND : " << it << " =================" << std::endl;

    // Start client
    client->Run();

    std::ofstream lossfile(output_csv_file, std::ofstream::out | std::ofstream::app);

    // Write statistic to csv
    std::cout << "Test losses:";
    lossfile << fetch::ml::utilities::GetStrTimestamp();
    auto *w2v_client = dynamic_cast<Word2VecClient<TensorType> *>(client.get());

    std::cout << "\t" << static_cast<double>(client->GetLossAverage()) << "\t"
              << w2v_client->GetAnalogyScore();
    lossfile << "\t" << static_cast<double>(client->GetLossAverage()) << "\t"
             << w2v_client->GetAnalogyScore();

    std::cout << std::endl;
    lossfile << std::endl;

    lossfile.close();
  }

  return 0;
}
