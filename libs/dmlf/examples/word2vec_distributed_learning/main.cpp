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

#include "dmlf/collective_learning/client_word2vec_algorithm.hpp"
#include "dmlf/collective_learning/utilities/utilities.hpp"
#include "dmlf/networkers/local_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "json/document.hpp"
#include "math/tensor.hpp"

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::dmlf::collective_learning;

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = typename TensorType::SizeType;

std::vector<std::string> SplitTrainingData(std::string const &train_file, SizeType n_clients)
{
  // split train file into n_clients parts
  std::vector<std::string> client_data;
  auto                     input_data       = fetch::ml::utilities::ReadFile(train_file);
  auto                     chars_per_client = static_cast<SizeType>(input_data.size() / n_clients);
  SizeType                 pos{0};
  SizeType                 old_pos{0};

  for (SizeType i(0); i < n_clients; ++i)
  {
    old_pos = pos;
    pos     = (i + 1) * chars_per_client;

    // find next instance of space character
    pos = input_data.find(" ", pos, 1);
    client_data.push_back(input_data.substr(old_pos, pos - old_pos));
  }
  return client_data;
}

/**
 * Averages weights between all clients
 * @param clients
 */
void SynchroniseWeights(std::vector<std::shared_ptr<CollectiveLearningClient<TensorType>>> clients)
{
  // gather all of the different clients' algorithms
  std::vector<std::shared_ptr<ClientAlgorithm<TensorType>>> client_algorithms;
  for (auto &client : clients)
  {
    std::vector<std::shared_ptr<ClientAlgorithm<TensorType>>> current_client_algorithms =
        client->GetAlgorithms();
    client_algorithms.insert(client_algorithms.end(), current_client_algorithms.begin(),
                             current_client_algorithms.end());
  }

  // Synchronize weights by giving all clients average of all client's weights
  std::vector<VectorTensorType>                  clients_weights{client_algorithms.size()};
  std::vector<fetch::byte_array::ConstByteArray> clients_vocab_hashes{client_algorithms.size()};

  for (SizeType i{0}; i < clients.size(); ++i)
  {
    clients_weights[i] = client_algorithms[i]->GetWeights();
    auto cast_client_i =
        std::dynamic_pointer_cast<Word2VecClient<TensorType>>(client_algorithms[i]);
    clients_vocab_hashes[i] = cast_client_i->GetVocab().second;
  }

  std::vector<VectorTensorType> clients_new_weights{client_algorithms.size()};

  for (SizeType i{0}; i < client_algorithms.size(); ++i)
  {
    VectorTensorType weights_new;

    auto cast_client_i =
        std::dynamic_pointer_cast<Word2VecClient<TensorType>>(client_algorithms[i]);

    for (SizeType k{0}; k < clients_weights.at(i).size(); ++k)
    {
      TensorType weight_sum;
      TensorType counts_sum;
      bool       first = true;
      for (SizeType j{0}; j < client_algorithms.size(); ++j)
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

int main(int argc, char **argv)
{
  // This example will create multiple local distributed clients with Word2Vec model and trains
  // word embeddings based on input text file

  if (argc != 2)
  {
    std::cout << "Usage : " << argv[0] << "config_file.json" << std::endl;
    return 1;
  }

  /**
   * Prepare configuration
   */

  fetch::json::JSONDocument doc;
  ClientParams<DataType>    client_params =
      fetch::dmlf::collective_learning::utilities::ClientParamsFromJson<TensorType>(
          std::string(argv[1]), doc);
  auto word2vec_client_params = std::make_shared<Word2VecTrainingParams<DataType>>(client_params);

  auto data_file                              = doc["data"].As<std::string>();
  word2vec_client_params->analogies_test_file = doc["analogies_test_file"].As<std::string>();
  word2vec_client_params->vocab_file          = doc["vocab_file"].As<std::string>();
  word2vec_client_params->test_frequency      = doc["test_frequency"].As<SizeType>();

  // Distributed learning parameters:
  auto        n_clients       = doc["n_clients"].As<SizeType>();
  auto        n_peers         = doc["n_peers"].As<SizeType>();
  auto        n_rounds        = doc["n_rounds"].As<SizeType>();
  auto        synchronise     = doc["synchronise"].As<bool>();
  std::string output_csv_file = doc["results"].As<std::string>();

  /**
   * Prepare environment
   */

  std::shared_ptr<std::mutex> console_mutex_ptr = std::make_shared<std::mutex>();
  std::cout << "FETCH Distributed Word2vec Demo" << std::endl;

  std::vector<std::string> client_data = SplitTrainingData(data_file, n_clients);
  std::vector<std::shared_ptr<CollectiveLearningClient<TensorType>>> clients(n_clients);
  std::vector<std::shared_ptr<fetch::dmlf::LocalLearnerNetworker>>   networkers(n_clients);

  // Create networkers
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers[i] = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
    networkers[i]->Initialize<fetch::dmlf::Update<TensorType>>();
  }

  // Add peers to networkers and initialise shuffle algorithm
  for (SizeType i(0); i < n_clients; ++i)
  {
    networkers[i]->AddPeers(networkers);
    networkers[i]->SetShuffleAlgorithm(std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(
        networkers[i]->GetPeerCount(), n_peers));
  }

  // Instantiate n_clients clients
  for (SizeType i(0); i < n_clients; ++i)
  {
    Word2VecTrainingParams<DataType> cp(*word2vec_client_params);
    cp.data    = {client_data[i]};
    clients[i] = std::make_shared<CollectiveLearningClient<TensorType>>(
        std::to_string(i), cp, networkers.at(i), console_mutex_ptr, false);

    // TODO - explicitly build word2vec algorithms in the client
    clients[i]
  }


  /**
   * Main loop
   */
  for (SizeType it(0); it < n_rounds; ++it)
  {
    // Start all clients
    std::cout << "================= ROUND : " << it << " =================" << std::endl;
    std::vector<std::thread> threads;
    for (auto &c : clients)
    {
      c->RunAlgorithms(threads);
    }

    // Wait for everyone to finish
    for (auto &t : threads)
    {
      t.join();
    }

    // Write statistic to csv
    std::ofstream lossfile(output_csv_file, std::ofstream::out | std::ofstream::app);

    std::cout << "Test losses:";
    lossfile << fetch::ml::utilities::GetStrTimestamp();
    for (auto &c : clients)
    {
      auto *w2v_client = dynamic_cast<Word2VecClient<TensorType> *>(c.get());

      std::cout << "\t" << static_cast<double>(c->GetLossAverage()) << "\t"
                << w2v_client->GetAnalogyScore();
      lossfile << "\t" << static_cast<double>(c->GetLossAverage()) << "\t"
               << w2v_client->GetAnalogyScore();
    }
    std::cout << std::endl;
    lossfile << std::endl;

    lossfile.close();

    // Synchronize weights by giving all clients average of all client's weights
    if (synchronise)
    {
      std::cout << std::endl << "Synchronising weights" << std::endl;
      SynchroniseWeights(clients);
    }
  }

  return 0;
}
