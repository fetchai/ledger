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

#include "file_loader.hpp"

//#include "math/ml/activation_functions/sigmoid.hpp"



//#include "model_saver.hpp"
//
//#include "math/clustering/knn.hpp"

#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
//
//#include "ml/dataloaders/word2vec_loaders/skipgram_dataloader.hpp"
//#include "ml/graph.hpp"
//#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"

#include <chrono>
#include <iostream>
#include <math/tensor.hpp>
#include <numeric>

using namespace fetch::ml;
//using namespace fetch::ml::dataloaders;
//using namespace fetch::ml::ops;
//using namespace fetch::ml::layers;
using DataType     = double;
using ArrayType    = fetch::math::Tensor<DataType>;
using ArrayPtrType = std::shared_ptr<ArrayType>;
using SizeType     = typename ArrayType::SizeType;

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  SizeType    output_size     = 1;
  SizeType    batch_size      = 500;            // training data batch size
  SizeType    embedding_size  = 200;            // dimension of embedding vec
  SizeType    training_epochs = 15;             // total number of training epochs
  double      learning_rate   = 0.025;          // alpha - the learning rate
  SizeType    k               = 10;             // how many nearest neighbours to compare against
  SizeType    print_freq      = 10000;          // how often to print status
  std::string test_word       = "action";       // test word to consider
  std::string save_loc        = "./model.fba";  // save file location for exporting graph
};

struct DataParams
{
  SizeType max_sentences      = 100000;
  SizeType max_sentence_len   = 500;
};


template <typename ArrayType>
class DataLoader
{
public:
    ArrayType data_;
    typename ArrayType::IteratorType *cursor_;
    typename ArrayType::IteratorType *context_cursor_;

    SizeType max_sentence_len_ = 0;

    std::unordered_map<std::string, SizeType> vocab_;             // unique vocab of words
    std::unordered_map<SizeType, SizeType>    vocab_frequencies;  // the count of each vocab word

    DataLoader(SizeType max_sentence_len, SizeType max_sentences) : data_({max_sentence_len, max_sentences})
    {
      max_sentence_len_ = max_sentence_len;
    }

    void AddData(std::string const &text)
    {
      reset_cursor();

      SizeType word_idx;
      std::string              word;
      for (std::stringstream s(text); s >> word;)
      {
        if (!((*cursor_).is_valid()))
        {
          break;
        }

        // if already seen this word
        auto it = vocab_.find(word);
        if (it != vocab_.end())
        {
          vocab_frequencies.at(it->second)++;
          word_idx = it->second;
        }
        else
        {
          word_idx = vocab_.size();
          vocab_frequencies.emplace(std::make_pair(vocab_.size(), SizeType(1)));
          vocab_.emplace(std::make_pair(word, vocab_.size()));
        }

        // write the word index to the data tensor
        *(*cursor_) = word_idx;
        ++(*cursor_);
      }

      // reset the cursor
      reset_cursor();
    }

    SizeType vocab_size()
    {
      return vocab_.size();
    }

    void next_positive(SizeType &input_idx, SizeType &context_idx)
    {
      input_idx = SizeType(*(*cursor_));
      ++(*cursor_);
      context_idx = SizeType(*(*cursor_));
    }

    bool done()
    {
      return ( (!((*cursor_).is_valid())) || (!((*context_cursor_).is_valid())) );
    }

    void reset_cursor()
    {
      cursor_ = new typename ArrayType::IteratorType(data_.begin());
      context_cursor_ = new typename ArrayType::IteratorType(data_.begin());

      for (std::size_t i = 0; i < max_sentence_len_; ++i)
      {
        ++(*context_cursor_);
      }
    }
};

/**
 * Read a single text file
 * @param path
 * @return
 */
std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

////////////////////////
/// MODEL DEFINITION ///
////////////////////////

template <typename ArrayType>
class SkipgramModel
{
  using Type = typename ArrayType::Type;

public:

  ArrayType input_embeddings_;

  ArrayType input_grads_;
  ArrayType context_grads_;

  ArrayType input_vector_;
  ArrayType context_vector_;
  ArrayType dot_result_;

  Type alpha_ = 0.2; // learning rate

  SkipgramModel(SizeType vocab_size, SizeType embeddings_size) : input_embeddings_({vocab_size, embeddings_size}), input_grads_({1, embeddings_size}), context_grads_({embeddings_size, 1})
  {
    // initialise embeddings to values from 0 - 0.1
    input_embeddings_.FillUniformRandom();
    fetch::math::Multiply(input_embeddings_, 0.1, input_embeddings_);
  }

  ArrayType Forward(SizeType input_word_idx, SizeType context_word_idx)
  {
    // embeddings input layer lookup
    input_vector_ = input_embeddings_.Slice(input_word_idx).Copy();

    // embeddings context layer lookup
    context_vector_ = input_embeddings_.Slice(context_word_idx).Copy();

    // context vector transpose
    // mat mul
    dot_result_ = fetch::math::DotTranspose(input_vector_, context_vector_);

    // sigmoid
    return Sigmoid(dot_result_);
  }

  void Backward(SizeType input_word_idx, SizeType context_word_idx, ArrayType& result, ArrayType &error)
  {
    // Sigmoid backprop
    Type tmp = fetch::math::Sigmoid(result);
    tmp*= (1 - tmp);
    error[0] *= tmp;

//    fetch::math::Multiply(fetch::math::Sigmoid(result), fetch::math::Subtract(1, fetch::math::Sigmoid(result), result), result);
//    error *= (result;

    // matmul backprop
    fetch::math::Dot(error, context_vector_, input_grads_); // no need to DotTranspose since we use DotTranspose instead of Dot in forward
    fetch::math::TransposeDot(input_vector_, error, context_grads_);

    // apply gradient updates
    auto input_slice_it = input_embeddings_.Slice(input_word_idx).begin();
    while (input_slice_it.is_valid())
    {
      *input_slice_it -= (input_grads_ * alpha_)[0];
      ++input_slice_it;
    }

    auto context_slice_it = input_embeddings_.Slice(context_word_idx).begin();
    while (context_slice_it.is_valid())
    {
      *context_slice_it -= (input_grads_ * alpha_)[0];
      ++context_slice_it;
    }
  }

private:

  Type Sigmoid(Type val)
  {
    if (val >= 0)
    {
      val *= -1;
      val = std::exp(val);
      val += 1;
      val = (1 / val);
    }
    else
    {
      val = std::exp(val);
      val = val / (val + 1);
    }
    return val
  }
};

int main(int argc, char **argv)
{

  std::string training_text;
  if (argc == 2)
  {
    training_text = argv[1];
  }
  else
  {
    throw std::runtime_error("must specify filename as training text");
  }

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  TrainingParams  tp;
  DataParams      dp;
//  SkipGramTextParams<ArrayType> sp = SetParams<ArrayType>();

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  std::cout << "Setting up training data...: " << std::endl;

  // set up dataloader
  DataLoader<ArrayType> dataloader(dp.max_sentence_len, dp.max_sentences);

  // load text from files as necessary and process text with dataloader
  dataloader.AddData(ReadFile(training_text));

  SizeType vocab_size = dataloader.vocab_size();
  std::cout << "vocab_size: " << vocab_size << std::endl;
//  std::cout << "dataloader.Size(): " << dataloader.Size() << std::endl;
//
  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  SkipgramModel<ArrayType> model(vocab_size, tp.embedding_size);


  std::cout << "begin training: " << std::endl;

  SizeType input_word_idx;
  SizeType context_word_idx;

  SizeType step_count{0};
  std::chrono::duration<double> time_diff;

  ArrayType gt({1, 1});
  ArrayType pred({1, 1});
  ArrayType loss({1, 1});

  auto t1 = std::chrono::high_resolution_clock::now();
  while (1)
  {

    if (dataloader.done())
    {
      dataloader.reset_cursor();
//
//      auto t2 = std::chrono::high_resolution_clock::now();
//      time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
//
//      std::cout << "words/sec: " << double(step_count) / time_diff.count() << std::endl;
//      t1 = std::chrono::high_resolution_clock::now();
//      step_count = 0;
    }

    ////////////////////////////////
    /// run one positive example ///
    ////////////////////////////////

    // get next data pair
    dataloader.next_positive(input_word_idx, context_word_idx);

    // forward pass on the model
    pred = model.Forward(input_word_idx, context_word_idx);

    // loss function
    loss[0] = fetch::math::CrossEntropyLoss(pred, gt);

    // backward pass
    model.Backward(input_word_idx, context_word_idx, pred, loss);

    ///////////////////////////////
    /// run k negative examples ///
    ///////////////////////////////
    
    ++step_count;


    if (step_count % 100 == 0)
    {

      auto t2 = std::chrono::high_resolution_clock::now();
      time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

      std::cout << "words/sec: " << double(step_count) / time_diff.count() << std::endl;
      t1 = std::chrono::high_resolution_clock::now();
      step_count = 0;
    }

  }

  return 0;
}
