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
#include "dmlf/networkers/local_learner_networker.hpp"
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

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::cout << "Usage : " << argv[0] << " PATH/TO/text8 analogies_test_file output_csv_file"
              << std::endl;
    return 1;
  }

  W2VTrainingParams<DataType> client_params;

  // Distributed learning parameters:
  SizeType number_of_clients = 5;
  SizeType number_of_rounds  = 1000;
  SizeType number_of_peers   = 3;
  bool     synchronisation   = false;
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

  std::string train_file            = argv[1];
  client_params.analogies_test_file = argv[2];

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
    networkers[i]->AddPeers(networkers);
    networkers[i]->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers[i]->GetPeerCount(), number_of_peers));
  }

  // Instantiate NUMBER_OF_CLIENTS clients
  for (SizeType i(0); i < number_of_clients; ++i)
  {
    W2VTrainingParams<DataType> cp = client_params;
    cp.data                        = {client_data[i]};
    auto client =
        std::make_shared<Word2VecClient<TensorType>>(std::to_string(i), cp, console_mutex_ptr);

    clients[i] = client;
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

    std::ofstream lossfile(std::string(argv[3]), std::ofstream::out | std::ofstream::app);

    std::cout << "Test losses:";
    lossfile << utilities::GetStrTimestamp();
    for (auto &c : clients)
    {
      auto *w2v_client = dynamic_cast<Word2VecClient<TensorType> *>(c.get());

      std::cout << "\t" << static_cast<double>(c->GetLossAverage()) << "\t"
                << w2v_client->analogy_score_;
      lossfile << "\t" << static_cast<double>(c->GetLossAverage()) << "\t"
               << w2v_client->analogy_score_;
    }
    std::cout << std::endl;
    lossfile << std::endl;

    lossfile.close();

    if (!synchronisation)
    {
      continue;
    }

    std::cout << std::endl << "Synchronising weights" << std::endl;

    // Synchronize weights by giving all clients average of all client's weights
    std::vector<VectorTensorType>                  clients_weights{clients.size()};
    std::vector<fetch::byte_array::ConstByteArray> clients_vocab_hashes{clients.size()};

    for (SizeType i{0}; i < number_of_clients; ++i)
    {
      clients_weights[i]      = clients[i]->GetWeights();
      auto cast_client_i      = std::dynamic_pointer_cast<Word2VecClient<TensorType>>(clients[i]);
      clients_vocab_hashes[i] = cast_client_i->GetVocab().second;
    }

    std::vector<VectorTensorType> clients_new_weights{clients.size()};

    for (SizeType i{0}; i < number_of_clients; ++i)
    {
      VectorTensorType weights_new;

      auto cast_client_i = std::dynamic_pointer_cast<Word2VecClient<TensorType>>(clients[i]);

      for (SizeType k{0}; k < clients_weights.at(i).size(); ++k)
      {
        TensorType weight_sum;
        TensorType counts_sum;
        bool       first = true;
        for (SizeType j{0}; j < number_of_clients; ++j)
        {
          auto ret =
              cast_client_i->TranslateWeights(clients_weights.at(j).at(k), clients_vocab_hashes[j]);
          if (first)
          {
            weight_sum = ret.first;
            counts_sum = ret.second;
            first      = false;
          }
          else
          {
            weight_sum += ret.first;
            counts_sum += ret.first;
          }
        }
        // divide weights by counts to get average
        weights_new.push_back(weight_sum / counts_sum);
      }

      cast_client_i->SetWeights(weights_new);
    }
  }

  return 0;
}
