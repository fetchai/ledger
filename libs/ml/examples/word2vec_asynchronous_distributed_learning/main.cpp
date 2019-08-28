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

#include "core/random.hpp"
#include "file_loader.hpp"
#include "math/clustering/knn.hpp"
#include "math/matrix_operations.hpp"
#include "math/statistics/mean.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "model_saver.hpp"


#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

// Runs in about 40 sec on a 2018 MBP
// Remember to disable debug using | grep -v INFO
#define NUMBER_OF_CLIENTS 3
#define NUMBER_OF_PEERS 2
#define NUMBER_OF_ITERATIONS 10
#define BATCH_SIZE 32
#define SYNCHRONIZATION_INTERVAL 3
#define MERGE_RATIO .5f
#define LEARNING_RATE .001f
#define TEST_SET_RATIO 0.03f

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

using DataType         = double;
using TensorType       = fetch::math::Tensor<DataType>;
using TensorVectorType = std::vector<TensorType>;
using SizeType         = typename TensorType::SizeType;

enum class CoordinatorState
{
  RUN,
  STOP,
};

class Coordinator
{
public:
  CoordinatorState state = CoordinatorState::RUN;
};

void PrintWordAnalogy(GraphW2VLoader<DataType> const &dl, TensorType const &embeddings,
                      std::string const &word1, std::string const &word2, std::string const &word3,
                      SizeType k)
{

  if (!dl.WordKnown(word1) || !dl.WordKnown(word2) || !dl.WordKnown(word3))
  {
    throw std::runtime_error("WARNING! not all to-be-tested words are in vocabulary");
  }
  else
  {
    std::cout << "Find word that to " << word3 << " is what " << word2 << " is to " << word1
              << std::endl;
  }

  // get id for words
  SizeType word1_idx = dl.IndexFromWord(word1);
  SizeType word2_idx = dl.IndexFromWord(word2);
  SizeType word3_idx = dl.IndexFromWord(word3);

  // get word vectors for words
  TensorType word1_vec = embeddings.Slice(word1_idx, 1).Copy();
  TensorType word2_vec = embeddings.Slice(word2_idx, 1).Copy();
  TensorType word3_vec = embeddings.Slice(word3_idx, 1).Copy();

  word1_vec /= fetch::math::L2Norm(word1_vec);
  word2_vec /= fetch::math::L2Norm(word2_vec);
  word3_vec /= fetch::math::L2Norm(word3_vec);

  TensorType word4_vec = word2_vec - word1_vec + word3_vec;

  std::vector<std::pair<typename TensorType::SizeType, typename TensorType::Type>> output =
      fetch::math::clustering::KNNCosine(embeddings, word4_vec, k);

  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": " << dl.WordFromIndex(output.at(l).first)
              << std::endl;
  }
}

void PrintKNN(GraphW2VLoader<DataType> const &dl, TensorType const &embeddings,
              std::string const &word0, SizeType k)
{
  if (dl.IndexFromWord(word0) == fetch::math::numeric_max<SizeType>())
  {
    throw std::runtime_error("WARNING! could not find [" + word0 + "] in vocabulary");
  }

  SizeType   idx        = dl.IndexFromWord(word0);
  TensorType one_vector = embeddings.Slice(idx, 1).Copy();
  std::vector<std::pair<typename TensorType::SizeType, typename TensorType::Type>> output =
      fetch::math::clustering::KNNCosine(embeddings, one_vector, k);

  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": " << dl.WordFromIndex(output.at(l).first)
              << std::endl;
  }
}

void TestEmbeddings(Graph<TensorType> const &g, std::string const &skip_gram_name,
                    GraphW2VLoader<DataType> const &dl, std::string const & word0,
                    std::string const & word1, std::string const & word2, std::string const & word3,
                    SizeType K)
{
  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<TensorType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<TensorType>>(
          (g.GetNode(skip_gram_name))->GetOp());

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<TensorType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  std::cout << std::endl;
  PrintKNN(dl, embeddings->get_weights(), word0, K);
  std::cout << std::endl;
  PrintWordAnalogy(dl, embeddings->get_weights(), word1, word2, word3, K);
}

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}
////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  SizeType max_word_count = fetch::math::numeric_max<SizeType>();  // maximum number to be trained
  SizeType negative_sample_size = 5;     // number of negative sample per word-context pair
  SizeType window_size          = 5;     // window size for context sampling
  DataType freq_thresh          = 1e-3;  // frequency threshold for subsampling
  SizeType min_count            = 5;     // infrequent word removal threshold

  SizeType batch_size      = 100000;  // training data batch size
  SizeType embedding_size  = 100;     // dimension of embedding vec
  SizeType training_epochs = 1;
  SizeType test_frequency  = 1;
  DataType starting_learning_rate_per_sample =
      0.00001;  // these are the learning rates we have for each sample
  DataType ending_learning_rate_per_sample = 0.000001;
  DataType starting_learning_rate;  // this is the true learning rate set for the graph training
  DataType ending_learning_rate;

  fetch::ml::optimisers::LearningRateParam<DataType> learning_rate_param{
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::LINEAR};

  SizeType    k        = 20;       // how many nearest neighbours to compare against
  std::string word0    = "three";  // test word to consider
  std::string word1    = "king";
  std::string word2    = "queen";
  std::string word3    = "father";
  std::string save_loc = "./model.fba";  // save file location for exporting graph
};

class TrainingClient
{
public:
  TrainingClient(TrainingParams const &tp, std::string const &vocab_file)
    : dataloader_(tp.window_size, tp.negative_sample_size, tp.freq_thresh, tp.max_word_count)
  {
    tp_ = tp;

    //    dataloader_.SetTestRatio(TEST_SET_RATIO);
    dataloader_.SetRandomMode(true);
    /// DATA LOADING ///
//    dataloader_.BuildVocab({ReadFile(train_file)}, tp_.min_count);
    dataloader_.LoadVocab(vocab_file);

    // calc the compatiable linear lr decay
    tp_.learning_rate_param.linear_decay_rate =
        static_cast<DataType>(1) /
        dataloader_
            .EstimatedSampleNumber();  // this decay rate gurantee the lr is reduced to zero by the
                                       // end of an epoch (despite capping by ending learning rate)
    std::cout << "dataloader_.EstimatedSampleNumber(): " << dataloader_.EstimatedSampleNumber()
              << std::endl;

    graph_ = std::make_shared<fetch::ml::Graph<TensorType>>();
    graph_->AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
    graph_->AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Context", {});
    label    = graph_->AddNode<PlaceHolder<TensorType>>("Label", {});
    skipgram = graph_->AddNode<fetch::ml::layers::SkipGram<TensorType>>(
        "SkipGram", {"Input", "Context"}, SizeType(1), SizeType(1), tp_.embedding_size,
        dataloader_.vocab_size());

    std::string error = graph_->AddNode<CrossEntropyLoss<TensorType>>("Error", {skipgram, label});

    // Initialise Optimiser
    optimiser_ = fetch::ml::optimisers::SGDOptimiser<TensorType>(
        graph_, {"Input", "Context"}, "Label", error, tp_.learning_rate_param);
  }

  void SetCoordinator(std::shared_ptr<Coordinator> &coordinator_ptr)
  {
    coordinator_ptr_ = coordinator_ptr;
  }

  void MainLoop()
  {
    DataType loss;
    std::cout << "beginning training...: " << std::endl;
    SizeType i{0};

    while (coordinator_ptr_->state == CoordinatorState::RUN)
    {
      std::cout << "start training for epoch no.: " << i << std::endl;
      // Create and apply own gradient
      optimiser_.Run(dataloader_, tp_.batch_size);

      if (i % tp_.test_frequency == 0)
      {
        TestEmbeddings(*graph_, skipgram, dataloader_, tp_.word0, tp_.word1, tp_.word2, tp_.word3,
                       tp_.k);
      }

      BroadcastGradients();

      // Process gradients queue
      TensorVectorType new_gradients;

      // Load own gradient - this has already been applied, so we replace it with zeros
      TensorVectorType my_gradients = GetGradients();
      TensorVectorType gradients;

      for (auto const &g : my_gradients)
      {
        gradients.emplace_back(TensorType(g.shape()));
      }

      // Sum all gradient in queue
      while (!gradient_queue.empty())
      {
        {
          std::lock_guard<std::mutex> l(queue_mutex_);
          new_gradients = gradient_queue.front();
          gradient_queue.pop();
        }

        for (SizeType j{0}; j < gradients.size(); j++)
        {
          fetch::math::Add(gradients.at(j), new_gradients.at(j), gradients.at(j));
        }
      }

      ApplyGradient(gradients);

      // Validate loss for logging purpose
      Test(loss);
      losses_values_.push_back(loss);

      // Shuffle the peers list to get new contact for next update
      fetch::random::Shuffle(gen_, peers_, peers_);
    }
  }

  //  DataType Train()
  //  {
  //
  //    dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  //
  //    DataType loss        = 0;
  //    bool     is_done_set = false;
  //
  //    std::pair<TensorType, std::vector<TensorType>> input;
  //    input = dataloader_.PrepareBatch(batch_size_, is_done_set);
  //
  //    {
  //      std::lock_guard<std::mutex> l(model_mutex_);
  //
  //      g_.SetInput("Input", input.second.at(0));
  //      g_.SetInput("Label", input.first);
  //
  //      TensorType loss_tensor = g_.ForwardPropagate("Error");
  //      loss                   = *(loss_tensor.begin());
  //      g_.BackPropagateError("Error");
  //    }
  //
  //    return loss;
  //  }

  void Test(DataType &test_loss)
  {
    dataloader_.SetMode(fetch::ml::dataloaders::DataLoaderMode::TEST);
    dataloader_.SetRandomMode(false);

    SizeType test_set_size = dataloader_.Size();

    dataloader_.Reset();
    bool is_done_set;
    auto test_pair = dataloader_.PrepareBatch(test_set_size, is_done_set);
    dataloader_.SetRandomMode(true);
    {
      std::lock_guard<std::mutex> l(model_mutex_);

      graph_->SetInput("Input", test_pair.second.at(0));
      graph_->SetInput("Label", test_pair.first);

      test_loss = *(graph_->ForwardPropagate("Error").begin());
    }
  }

  TensorVectorType GetGradients() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    return graph_->GetGradients();
  }

  TensorVectorType GetWeights() const
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    return graph_->get_weights();
  }

  void AddPeers(std::vector<std::shared_ptr<TrainingClient>> const &clients)
  {
    for (auto const &p : clients)
    {
      if (p.get() != this)
      {
        peers_.push_back(p);
      }
    }
    fetch::random::Shuffle(gen_, peers_, peers_);
  }

  void BroadcastGradients()
  {
    // Load own gradient
    TensorVectorType current_gradient = graph_->GetGradients();

    // Give gradients to peers
    for (unsigned int i(0); i < NUMBER_OF_PEERS; ++i)
    {
      peers_[i]->AddGradient(current_gradient);
    }
  }

  void AddGradient(TensorVectorType gradient)
  {
    {
      std::lock_guard<std::mutex> l(queue_mutex_);
      gradient_queue.push(gradient);
    }
  }

  void ApplyGradient(TensorVectorType gradients)
  {
    // SGD
    for (SizeType j{0}; j < gradients.size(); j++)
    {
      fetch::math::Multiply(gradients.at(j), -LEARNING_RATE, gradients.at(j));
    }

    // Apply gradients
    {
      std::lock_guard<std::mutex> l(model_mutex_);
      graph_->ApplyGradients(gradients);
    }
  }

  void SetWeights(TensorVectorType &new_weights)
  {
    std::lock_guard<std::mutex> l(const_cast<std::mutex &>(model_mutex_));
    graph_->set_weights(new_weights);
  }

  std::vector<DataType> const &GetLossesValues() const
  {
    return losses_values_;
  }

private:
  // Client own graph
  std::shared_ptr<fetch::ml::Graph<TensorType>> graph_;

  TrainingParams tp_;

  // Client own dataloader
  GraphW2VLoader<DataType> dataloader_;

  // optimiser
  fetch::ml::optimisers::SGDOptimiser<TensorType> optimiser_;

  // Loss history
  std::vector<DataType> losses_values_;

  // Connection to other nodes
  std::vector<std::shared_ptr<TrainingClient>> peers_;

  // Mutex to protect weight access
  std::mutex model_mutex_;
  std::mutex queue_mutex_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;

  //  SizeType batch_size_ = BATCH_SIZE;

  std::queue<TensorVectorType> gradient_queue;

  std::shared_ptr<Coordinator> coordinator_ptr_;

  std::string label;
  std::string skipgram;
};

int main(int ac, char **av)
{
  if (ac < 2)
  {
    std::cout << "Usage : " << av[0] << " PATH/TO/text8" << std::endl;
    return 1;
  }

  std::string train_file = av[1];

  std::shared_ptr<Coordinator> coordinator = std::make_shared<Coordinator>();

  std::cout << "FETCH Distributed Word2vec Demo -- Asynchronous" << std::endl;
  std::srand((unsigned int)std::time(nullptr));

  TrainingParams tp;

  // calc the true starting learning rate
  tp.starting_learning_rate =
      static_cast<DataType>(tp.batch_size) * tp.starting_learning_rate_per_sample;
  tp.ending_learning_rate =
      static_cast<DataType>(tp.batch_size) * tp.ending_learning_rate_per_sample;
  tp.learning_rate_param.starting_learning_rate = tp.starting_learning_rate;
  tp.learning_rate_param.ending_learning_rate   = tp.ending_learning_rate;


  GraphW2VLoader<DataType> data_loader(tp.window_size, tp.negative_sample_size, tp.freq_thresh,
                                       tp.max_word_count);
  // set up dataloader
  /// DATA LOADING ///
  std::string vocab_file = "/tmp/vocab.txt";
  data_loader.BuildVocab({ReadFile(train_file)}, tp.min_count);
  data_loader.SaveVocab(vocab_file);

  std::vector<std::shared_ptr<TrainingClient>> clients(NUMBER_OF_CLIENTS);
  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Instanciate NUMBER_OF_CLIENTS clients
    clients[i] = std::make_shared<TrainingClient>(tp, vocab_file);
  }

  for (unsigned int i(0); i < NUMBER_OF_CLIENTS; ++i)
  {
    // Give every client the full list of other clients
    clients[i]->AddPeers(clients);

    // Give each client pointer to coordinator
    clients[i]->SetCoordinator(coordinator);
  }

  // Main loop
  for (unsigned int it(0); it < NUMBER_OF_ITERATIONS; ++it)
  {

    // Start all clients
    coordinator->state = CoordinatorState::RUN;
    std::cout << "================= ITERATION : " << it << " =================" << std::endl;
    std::list<std::thread> threads;
    for (auto &c : clients)
    {
      threads.emplace_back([&c] { c->MainLoop(); });
    }

    std::this_thread::sleep_for(std::chrono::seconds(SYNCHRONIZATION_INTERVAL));

    // Send stop signal to all clients
    coordinator->state = CoordinatorState::STOP;

    // Wait for everyone to finish
    for (auto &t : threads)
    {
      t.join();
    }

    // Synchronize weights
    TensorVectorType weights = clients[0]->GetWeights();
    for (unsigned int i(1); i < NUMBER_OF_CLIENTS; ++i)
    {
      clients[i]->SetWeights(weights);
    }
  }

  // Save loss variation data
  // Upload to https://plot.ly/create/#/ for visualisation
  std::ofstream lossfile("losses.csv", std::ofstream::out | std::ofstream::trunc);
  if (lossfile)
  {
    for (unsigned int i(0); i < clients.size(); ++i)
    {
      lossfile << "Client " << i << ", ";

      for (unsigned int j(0); j < clients.at(i)->GetLossesValues().size(); ++j)
      {
        lossfile << clients.at(i)->GetLossesValues()[j] << ", ";
      }

      lossfile << "\n";
    }
  }
  lossfile.close();

  return 0;
}
