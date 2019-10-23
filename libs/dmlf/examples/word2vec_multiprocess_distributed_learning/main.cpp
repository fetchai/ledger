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

#include "dmlf/distributed_learning/word2vec_client.hpp"
#include "dmlf/networkers/muddle_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"

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

int main(int argc, char **argv)
{
  if (argc != 6)
  {
    std::cout
        << "Usage : " << argv[0]
        << " PATH/TO/text8 analogies_test_file output_csv_file networker_config instance_number"
        << std::endl;
    return 1;
  }

  W2VTrainingParams<DataType> client_params;

  // Command line parameters
  std::string train_file            = argv[1];
  client_params.analogies_test_file = argv[2];
  std::string output_csv_file       = argv[3];
  std::string config                = std::string(argv[4]);
  int         instance_number       = std::atoi(argv[5]);

  // Distributed learning parameters:
  SizeType number_of_rounds = 1000;
  SizeType number_of_peers  = 3;
  // have been processed in total by the clients

  client_params.batch_size    = 10000;
  client_params.learning_rate = static_cast<DataType>(.001f);
  client_params.max_updates   = 100;  // Round ends after this number of batches

  // Word2Vec parameters:
  client_params.vocab_file           = "/tmp/vocab.txt";
  client_params.negative_sample_size = 5;  // number of negative sample per word-context pair
  client_params.window_size          = 5;  // window size for context sampling
  client_params.freq_thresh          = DataType{0.001f};  // frequency threshold for subsampling
  client_params.min_count            = 5;                 // infrequent word removal threshold
  client_params.embedding_size       = 100;               // dimension of embedding vec
  client_params.starting_learning_rate_per_sample =
      DataType{0.0025f};  // these are the learning rates we have for each sample
  client_params.test_frequency = 1000;

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

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();
  std::cout << "FETCH Distributed Word2vec Demo" << std::endl;

  std::string client_data = ReadFile(train_file);

  W2VTrainingParams<DataType> cp = client_params;
  cp.data                        = {client_data};

  auto client    = std::make_shared<Word2VecClient<TensorType>>(std::to_string(instance_number), cp,
                                                             console_mutex_ptr);
  auto networker = std::make_shared<fetch::dmlf::MuddleLearnerNetworker>(config, instance_number);
  networker->Initialize<fetch::dmlf::Update<TensorType>>();

  networker->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
      networker->GetPeerCount(), number_of_peers));

  client->SetNetworker(networker);

  // Main loop
  for (SizeType it(0); it < number_of_rounds; ++it)
  {

    // Start all clients
    std::cout << "================= ROUND : " << it << " =================" << std::endl;

    client->Run();

    std::ofstream lossfile(output_csv_file, std::ofstream::out | std::ofstream::app);

    std::cout << "Test losses:";
    lossfile << utilities::GetStrTimestamp();
    auto *w2v_client = dynamic_cast<Word2VecClient<TensorType> *>(client.get());

    std::cout << "\t" << static_cast<double>(client->GetLossAverage()) << "\t"
              << w2v_client->analogy_score_;
    lossfile << "\t" << static_cast<double>(client->GetLossAverage()) << "\t"
             << w2v_client->analogy_score_;

    std::cout << std::endl;
    lossfile << std::endl;

    lossfile.close();
  }

  return 0;
}
