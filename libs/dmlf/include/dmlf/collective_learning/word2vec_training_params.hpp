#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/optimisation/learning_rate_params.hpp"

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

namespace fetch {
namespace dmlf {
namespace collective_learning {

template <class DataType>
struct Word2VecTrainingParams : public ClientParams<DataType>
{
  using SizeType = fetch::math::SizeType;

  // Overwriting base client default params
  explicit Word2VecTrainingParams(ClientParams<DataType> &cp)
    : ClientParams<DataType>(cp)
  {
    this->batch_size = 10000;

    // Initialise default values

    // calc the true starting learning rate
    starting_learning_rate_per_sample =
        static_cast<DataType>(this->batch_size) * starting_learning_rate_per_sample;
    ending_learning_rate =
        static_cast<DataType>(this->batch_size) * ending_learning_rate_per_sample;
    learning_rate_param.starting_learning_rate = starting_learning_rate;
    learning_rate_param.ending_learning_rate   = ending_learning_rate;
  }

  SizeType max_word_count = fetch::math::numeric_max<SizeType>();  // maximum number to be trained
  SizeType negative_sample_size = 5;  // number of negative sample per word-context pair
  SizeType window_size          = 5;  // window size for context sampling
  DataType freq_thresh =
      fetch::math::Type<DataType>("0.001");  // frequency threshold for subsampling
  SizeType min_count = 5;                    // infrequent word removal threshold

  SizeType embedding_size = 100;  // dimension of embedding vec
  SizeType test_frequency = 50;   // After how many batches we want to test our embeddings
  DataType starting_learning_rate_per_sample = fetch::math::Type<DataType>(
      "0.0025");  // these are the learning rates we have for each sample
  DataType    ending_learning_rate_per_sample = fetch::math::Type<DataType>("0.0001");
  DataType    starting_learning_rate;  // this is the true learning rate set for the graph training
  DataType    ending_learning_rate;
  std::string vocab_file;
  std::vector<std::string> data;
  std::string              analogies_test_file;

  fetch::ml::optimisers::LearningRateParam<DataType> learning_rate_param{
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::LINEAR};

  SizeType    k        = 20;       // how many nearest neighbours to compare against
  std::string word0    = "three";  // test word to consider
  std::string word1    = "king";
  std::string word2    = "queen";
  std::string word3    = "father";
  std::string save_loc = "./model.fba";  // save file location for exporting graph
};

}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
