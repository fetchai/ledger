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
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/distributed_learning/word2vec_client.hpp"

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

std::vector<std::string> SplitTrainingData(std::string const &train_file,
                                           SizeType           number_of_clients)
{
  // split train file into number_of_clients parts
  std::vector<std::string> client_data;
  auto                     input_data = ReadFile(train_file);
  auto     chars_per_client = static_cast<SizeType>(input_data.size() / number_of_clients);
  SizeType pos{0};
  SizeType old_pos{0};

  for (SizeType i(0); i < number_of_clients; ++i)
  {
    old_pos = pos;
    pos     = (i + 1) * chars_per_client;

    // find next instance of space character
    pos = input_data.find(" ", pos, 1);
    client_data.push_back(input_data.substr(old_pos, pos - old_pos));
  }
  return client_data;
}

void MakeVocabFile(W2VTrainingParams<DataType> const &client_params, std::string const &train_file)
{
  GraphW2VLoader<DataType> data_loader(client_params.window_size,
                                       client_params.negative_sample_size,
                                       client_params.freq_thresh, client_params.max_word_count);
  data_loader.BuildVocabAndData({ReadFile(train_file)}, client_params.min_count, false);
  data_loader.SaveVocab(client_params.vocab_file);
}

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cout << "Usage : " << argv[0] << " PATH/TO/text8 analogies_test_file" << std::endl;
    return 1;
  }

  W2VTrainingParams<DataType> client_params;

  // Distributed learning parameters:
  SizeType number_of_clients = 5;
  SizeType number_of_rounds  = 50;
  SizeType number_of_peers   = 2;

  //  Synchronization occurs after this number of batches have been processed in total by the
  //  clients
  client_params.iterations_count = 100;
  client_params.batch_size       = 10000;
  client_params.learning_rate    = static_cast<DataType>(.001f);

  // Word2Vec parameters:
  client_params.vocab_file           = "/tmp/vocab.txt";
  client_params.negative_sample_size = 5;  // number of negative sample per word-context pair
  client_params.window_size          = 5;  // window size for context sampling
  client_params.freq_thresh          = DataType{0.001f};  // frequency threshold for subsampling
  client_params.min_count            = 5;                 // infrequent word removal threshold
  client_params.embedding_size       = 100;               // dimension of embedding vec
  client_params.starting_learning_rate_per_sample =
      0.0025;  // these are the learning rates we have for each sample

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

  std::cout << "FETCH Distributed Word2vec Demo -- Asynchronous" << std::endl;

  std::string train_file            = argv[1];
  client_params.analogies_test_file = argv[2];

  MakeVocabFile(client_params, train_file);

  std::vector<std::string> client_data = SplitTrainingData(train_file, number_of_clients);

  std::vector<std::shared_ptr<TrainingClient<TensorType>>>         clients(number_of_clients);
  std::vector<std::shared_ptr<fetch::dmlf::LocalLearnerNetworker>> networkers(number_of_clients);

  // Create networkers
  for (SizeType i(0); i < number_of_clients; ++i)
  {
    networkers[i] = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
    networkers[i]->Initialize<fetch::dmlf::Update<TensorType>>();
  }

  for (SizeType i(0); i < number_of_clients; ++i)
  {
    networkers[i]->addPeers(networkers);
    networkers[i]->setShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers[i]->getPeerCount(), number_of_peers));
  }

  for (SizeType i(0); i < number_of_clients; ++i)
  {
    W2VTrainingParams<DataType> cp = client_params;
    cp.data                        = {client_data[i]};
    // Instantiate NUMBER_OF_CLIENTS clients
    clients[i] =
        std::make_shared<Word2VecClient<TensorType>>(std::to_string(i), cp, console_mutex_ptr);
    // TODO(1597): Replace ID with something more sensible
  }

  for (SizeType i(0); i < number_of_clients; ++i)
  {
    // Give each client pointer to coordinator
    clients[i]->SetNetworker(networkers[i]);
  }

  // Main loop
  for (SizeType it(0); it < number_of_rounds; ++it)
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
  }

  return 0;
}
