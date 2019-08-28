#pragma once
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

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

template <class TensorType>
struct TrainingParams
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

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
