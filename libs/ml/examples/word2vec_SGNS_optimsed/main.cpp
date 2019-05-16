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

#include "core/random/lcg.hpp"
#include "file_loader.hpp"
#include "math/clustering/knn.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"

#include <chrono>
#include <iostream>
#include <math/tensor.hpp>
#include <numeric>

#define TRAINING_WORDS 17000000

// TODO: implement frequency table for selecting negative samples proportionally
// TODO: batch training
// TODO: if the dataloader data_ array is different in size from the number of words in the data
// added, there will be empty 0 values used in training

// TODO: decide whether to include l2 regularisation

using namespace fetch::ml;
using DataType     = double;
using ArrayType    = fetch::math::Tensor<DataType>;
using ArrayPtrType = std::shared_ptr<ArrayType>;
using SizeType     = typename ArrayType::SizeType;

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  SizeType output_size       = 1;
  SizeType batch_size        = 500;       // training data batch size
  SizeType embedding_size    = 200;       // dimension of embedding vec
  SizeType training_epochs   = 15;        // total number of training epochs
  SizeType neg_examples      = 25;        // how many negative examples for every positive example
  double   learning_rate     = 0.2;       // alpha - the learning rate
  double   min_learning_rate = 0.000005;  // alpha - the minimum learning rate
  double   negative_learning_rate;        // alpha - the learning rate for negative examples
  double   min_negative_learning_rate;    // alpha - the minimum learning rate for negative examples

  SizeType    k          = 10;             // how many nearest neighbours to compare against
  SizeType    print_freq = 100000;         // how often to print status
  std::string test_word  = "action";       // test word to consider
  std::string save_loc   = "./model.fba";  // save file location for exporting graph

  SizeType max_sentences    = 10000;  // maximum sentences for dataloader
  SizeType max_sentence_len = 1700;   // maximum sentence length for dataloader
  SizeType window_size      = 5;      // one side of the context window

  SizeType total_words = TRAINING_WORDS * training_epochs;
};

template <typename ArrayType>
class DataLoader
{
public:
  using IteratorType = typename ArrayType::IteratorType;

  ///
  /// Data & cursors
  ///

  ArrayType    data_;
  SizeType     cursor_offset_;
  SizeType     n_positive_cursors_;
  IteratorType cursor_ = {data_};

  // positive cursors
  std::vector<IteratorType> positive_cursors_;

  ///
  /// Random values
  ///

  fetch::random::LinearCongruentialGenerator gen;
  SizeType                                   ran_val_;
  //  SizeType const                             N_RANDOM_ROWS = 1000000;
  std::vector<SizeType> ran_positive_cursor_{1000000};
  std::vector<SizeType> rows_{};

  SizeType max_sentence_len_ = 0;

  std::unordered_map<std::string, SizeType> vocab_;             // unique vocab of words
  std::unordered_map<SizeType, SizeType>    vocab_frequencies;  // the count of each vocab word

  DataLoader(SizeType max_sentence_len, SizeType max_sentences, SizeType window_size)
    : data_({max_sentence_len, max_sentences})
    , cursor_offset_(window_size)
    , n_positive_cursors_(2 * window_size)
    , max_sentence_len_(max_sentence_len)
  {
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      positive_cursors_.emplace_back(IteratorType(data_));
    }
    //    for (std::size_t j = 0; j < N_RANDOM_ROWS; ++j)
    //    {
    //      ran_positive_cursor_.emplace_back
    //
    //    }

    PrepareDynamicWindowProbs();
  }

  void AddData(std::string const &text)
  {
    cursor_ = data_.begin();

    SizeType    word_idx;
    std::string word;
    for (std::stringstream s(text); s >> word;)
    {
      if (!(cursor_.is_valid()))
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
      *cursor_ = word_idx;
      ++cursor_;
    }

    // reset the cursor
    reset_cursor();
  }

  SizeType vocab_size()
  {
    return vocab_.size();
  }

  SizeType vocab_lookup(std::string &word)
  {
    return vocab_[word];
  }

  std::string VocabLookup(SizeType word_idx)
  {
    auto vocab_it = vocab_.begin();
    while (vocab_it != vocab_.end())
    {
      if (vocab_it->second == word_idx)
      {
        return vocab_it->first;
      }
      ++vocab_it;
    }
    return "UNK";
  }

  void next_positive(SizeType &input_idx, SizeType &context_idx)
  {
    input_idx = static_cast<SizeType>(*cursor_);

    // generate random value from 0 -> 2xwindow_size
    ran_val_ = gen() % ran_positive_cursor_.size();

    // dynamic context window - pick positive cursor
    context_idx = static_cast<SizeType>(*(positive_cursors_[ran_positive_cursor_[ran_val_]]));
  }

  void next_negative(SizeType &input_idx, SizeType &context_idx)
  {
    input_idx = static_cast<SizeType>(*cursor_);

    // randomly select a negative cursor
    context_idx =
        static_cast<SizeType>(data_.At(gen() % data_.shape()[0], gen() % data_.shape()[1]));
  }

  void IncrementCursors()
  {
    ++cursor_;
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      ++(positive_cursors_[i]);
    }
  }

  bool done()
  {
    // we just check that the final positive window cursor isn't valid
    return (!(positive_cursors_[n_positive_cursors_ - 1].is_valid()));
  }

  void reset_cursor()
  {
    // the data cursor is on the current index
    cursor_ = IteratorType(data_, cursor_offset_);

    // set up the positive cursors and shift them
    for (std::size_t j = 0; j < 2 * cursor_offset_; ++j)
    {
      if (j < cursor_offset_)
      {
        positive_cursors_[j] = IteratorType(data_, cursor_offset_ - (cursor_offset_ - j));
      }
      else
      {
        positive_cursors_[j] = IteratorType(data_, cursor_offset_ + (j - cursor_offset_ + 1));
      }
    }

    ASSERT(cursor_.is_valid());
    for (std::size_t l = 0; l < n_positive_cursors_; ++l)
    {
      ASSERT(positive_cursors_[l].is_valid());
    }
  }

  void PrepareDynamicWindowProbs()
  {
    // get sum of frequencies for dynamic window
    SizeType sum_freqs = 0;
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      if (i < cursor_offset_)
      {
        sum_freqs += i + 1;
      }
      else
      {
        sum_freqs += (cursor_offset_ - (i - cursor_offset_));
      }
    }

    // calculate n rows per cursor
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      if (i < cursor_offset_)
      {
        for (std::size_t j = 0;
             j < (double(i + 1) / double(sum_freqs)) * ran_positive_cursor_.size(); ++j)
        {
          rows_.emplace_back(i);
        }
      }
      else
      {
        for (std::size_t j = 0;
             j < (double((cursor_offset_ - (i - cursor_offset_))) / double(sum_freqs)) *
                     ran_positive_cursor_.size();
             ++j)
        {
          rows_.emplace_back(i);
        }
      }
    }

    // move from vector into fixed array
    for (std::size_t k = 0; k < ran_positive_cursor_.size(); ++k)
    {
      if (k < rows_.size())
      {
        ran_positive_cursor_[k] = rows_[k];
      }

      // TODO - this probably doesn't work as expected
      // if the vector and array are different sizes, just fill up the extra entries with high
      // probability cases
      else
      {
        ran_positive_cursor_[k] = cursor_offset_;
      }
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
  ArrayType result_{{1, 1}};
  ArrayType dot_result_{{1, 1}};

  Type alpha_ = 0.2;  // learning rate

  std::vector<Type> l2reg_row_sums;
  Type              l2reg_sum = 0;
  Type              l2_lambda = 0.0001;

  SkipgramModel(SizeType vocab_size, SizeType embeddings_size, Type learning_rate)
    : input_embeddings_({vocab_size, embeddings_size})
    , input_grads_({1, embeddings_size})
    , context_grads_({embeddings_size, 1})
    , alpha_(learning_rate)
    , l2reg_row_sums{std::vector<Type>(vocab_size, 0)}
  {
    // initialise embeddings to values from 0 - 0.1
    input_embeddings_.FillUniformRandom();
    fetch::math::Multiply(input_embeddings_, 0.1, input_embeddings_);
    fetch::math::Subtract(input_embeddings_, 0.05, input_embeddings_);

    // initial embeddings weight normalisation
    Type sum = 0;
    for (auto &e : input_embeddings_)
    {
      sum += (e * e);
    }
    for (auto &e : input_embeddings_)
    {
      e /= sum;
    }

    // initialise the l2 normalisation values
    for (std::size_t i = 0; i < vocab_size; ++i)
    {
      auto it = input_embeddings_.Slice(i).begin();
      while (it.is_valid())
      {
        l2reg_row_sums[i] += (*it * *it);
        ++it;
      }
      l2reg_sum += l2reg_row_sums[i];
    }
  }

  void NormaliseEmbeddingRow(SizeType row)
  {
    auto it = input_embeddings_.Slice(row).begin();
    while (it.is_valid())
    {
      *it /= l2reg_sum;
      ++it;
    }
  }

  /**
   * This function combines the forward pass and loss calculation
   * for a positive context example (gt == 1):
   * x = v_in' * v_out
   * l = log(sigmoid(x))
   *
   * for a negative context example (gt == 0):
   * x = v_in' * v_sample
   * l = log(sigmoid(-x))
   */
  void ForwardAndLoss(SizeType input_word_idx, SizeType context_word_idx, Type gt, Type &loss)
  {
    // First normalise the embeddings. Since that's expensive, we just normalise the two rows
    // we'll use
    NormaliseEmbeddingRow(input_word_idx);
    NormaliseEmbeddingRow(context_word_idx);

    // TODO - remove these copies when math library SFINAE checks for IsIterable instead of taking
    // arrays. embeddings input & context lookup
    input_vector_   = input_embeddings_.Slice(input_word_idx).Copy();
    context_vector_ = input_embeddings_.Slice(context_word_idx).Copy();

    // context vector transpose
    // mat mul
    fetch::math::DotTranspose(input_vector_, context_vector_, dot_result_);

    ASSERT(result_.shape()[0] == 1);
    ASSERT(result_.shape()[1] == 1);

    if (std::isnan(dot_result_[0]))
    {
      std::cout << "input_vector_.ToString(): " << input_vector_.ToString() << std::endl;
      std::cout << "context_vector_.ToString(): " << context_vector_.ToString() << std::endl;
      throw std::runtime_error("dot_result_ is nan");
    }

    ///////
    /// 1 - sigmoid(x) == sigmoid(-x)
    ///////

    // sigmoid_cross_entropy_loss
    Sigmoid(dot_result_[0], result_[0]);

    if (gt == 1)
    {
      if (result_[0] <= 0)
      {
        throw std::runtime_error("cant take log of negative values");
      }
      loss = -std::log(result_[0]);
    }
    else
    {
      if ((1 - result_[0]) <= 0)
      {
        throw std::runtime_error("cant take log of negative values");
      }
      loss = -std::log(1 - result_[0]);
    }

    loss += (l2_lambda * l2reg_sum);

    if (std::isnan(loss))
    {
      throw std::runtime_error("loss is nan");
    }
  }

  void Backward(SizeType const &input_word_idx, SizeType const &context_word_idx, Type const &gt)
  {
    // positive case:
    // dl/dx = g = sigmoid(-x)
    // dl/d(v_in) = g * v_out'
    // dl/d(v_out) = v_in' * g
    //
    // negative case:
    // dl/dx = g = -sigmoid(x)
    // dl/d(v_in) = g * v_out'
    // dl/d(v_out) = v_in' * g

    // multiply by learning rate
    result_[0] = (gt - result_[0]) * alpha_;

    // calculate dl/d(v_in)
    fetch::math::Multiply(context_vector_, result_[0], input_grads_);

    // calculate dl/d(v_out)
    fetch::math::Multiply(input_vector_, result_[0], context_grads_);

    // apply gradient updates
    l2reg_sum -= l2reg_row_sums[input_word_idx];
    l2reg_sum -= l2reg_row_sums[context_word_idx];

    l2reg_row_sums[input_word_idx]   = 0;
    l2reg_row_sums[context_word_idx] = 0;

    auto input_slice_it = input_embeddings_.Slice(input_word_idx).begin();
    auto input_grads_it = input_grads_.begin();
    while (input_slice_it.is_valid())
    {
      // grad and l2_reg weight decay
      *input_slice_it += (*input_grads_it) - (l2_lambda * *input_slice_it);
      //      *input_slice_it += (*input_grads_it);
      l2reg_row_sums[input_word_idx] += (*input_slice_it * *input_slice_it);
      ++input_slice_it;
      ++input_grads_it;
    }

    // apply gradient updates
    auto context_slice_it = input_embeddings_.Slice(context_word_idx).begin();
    auto context_grads_it = context_grads_.begin();
    while (context_slice_it.is_valid())
    {
      // grad and l2_reg weight decay
      *context_slice_it += (*context_grads_it) - (l2_lambda * *input_slice_it);
      //      *context_slice_it += (*context_grads_it);
      l2reg_row_sums[context_word_idx] += (*context_slice_it * *context_slice_it);
      ++context_slice_it;
      ++context_grads_it;
    }

    l2reg_sum += l2reg_row_sums[input_word_idx];
    l2reg_sum += l2reg_row_sums[context_word_idx];
  }

  void Sigmoid(Type x, Type &ret)
  {
    // sigmoid
    ret = (1 / (1 + std::exp(-x)));

    // clamping function ensures numerical stability so we don't take log(0)
    fetch::math::Max(ret, epsilon_, ret);
    fetch::math::Min(ret, 1 - epsilon_, ret);

    if (std::isnan(ret))
    {
      throw std::runtime_error("ret is nan");
    }
  }

  Type epsilon_ = 1e-7;
};

////////////////////
/// EVAL ANALOGY ///
////////////////////

void EvalAnalogy(DataLoader<ArrayType> &dl, SkipgramModel<ArrayType> &model)
{
  SizeType    k     = 5;

  SizeType word1_idx;
  SizeType word2_idx;
  SizeType word3_idx;

  //
  std::string word1 = "Italy";
  std::string word2 = "Rome";
  std::string word3 = "France";
  std::string word4 = "Paris";

  // vector math - hopefully target_vector is close to the location of the embedding value for word4
  word1_idx     = dl.vocab_lookup(word1);
  word2_idx     = dl.vocab_lookup(word2);
  word3_idx     = dl.vocab_lookup(word3);
  auto     target_vector = model.input_embeddings_.Slice(word3_idx).Copy() +
                       (model.input_embeddings_.Slice(word2_idx).Copy() -
                        model.input_embeddings_.Slice(word1_idx).Copy());

  // cosine distance between every word in vocab and the target vector
  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
      fetch::math::clustering::KNNCosine(model.input_embeddings_, target_vector, k);

  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << dl.VocabLookup(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;





}

///////////////
///  main  ////
///////////////

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

  TrainingParams tp;

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  std::cout << "Setting up training data...: " << std::endl;

  // set up dataloader
  DataLoader<ArrayType> dataloader(tp.max_sentence_len, tp.max_sentences, tp.window_size);

  // load text from files as necessary and process text with dataloader
  dataloader.AddData(ReadFile(training_text));

  SizeType vocab_size = dataloader.vocab_size();
  std::cout << "vocab_size: " << vocab_size << std::endl;

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  SkipgramModel<ArrayType> model(vocab_size, tp.embedding_size, tp.learning_rate);
  tp.negative_learning_rate     = tp.learning_rate / tp.neg_examples;
  tp.min_negative_learning_rate = tp.min_learning_rate / tp.neg_examples;

  std::cout << "begin training: " << std::endl;

  SizeType input_word_idx;
  SizeType context_word_idx;

  SizeType                      step_count{0};
  SizeType                      cursor_idx{0};
  SizeType                      total_step_count{0};
  std::chrono::duration<double> time_diff;

  DataType gt;
  DataType loss     = 0;
  DataType sum_loss = 0;

  SizeType epoch_count = 0;

  auto t1 = std::chrono::high_resolution_clock::now();
  while (epoch_count < tp.training_epochs)
  {
    if (dataloader.done())
    {
      std::cout << "end of epoch: " << epoch_count << std::endl;
      ++epoch_count;

      dataloader.reset_cursor();
      cursor_idx = 0;

      std::cout << "testing analogies: " << std::endl;
      EvalAnalogy(dataloader, model);
    }

    auto one_min_completed_train_fraction =
        1.0 - (static_cast<double>(cursor_idx) / tp.total_words);

    ////////////////////////////////
    /// run one positive example ///
    ////////////////////////////////
    gt           = 1;
    model.alpha_ = (tp.learning_rate *
                    fetch::math::Max(tp.min_learning_rate, one_min_completed_train_fraction));

    // get next data pair
    dataloader.next_positive(input_word_idx, context_word_idx);

    // forward pass on the model & loss calculation bundled together
    model.ForwardAndLoss(input_word_idx, context_word_idx, gt, loss);

    // backward pass
    model.Backward(input_word_idx, context_word_idx, gt);
    ++step_count;
    ++total_step_count;

    sum_loss += (loss * model.alpha_);

    ///////////////////////////////
    /// run k negative examples ///
    ///////////////////////////////
    gt           = 0;
    model.alpha_ = (tp.negative_learning_rate * fetch::math::Max(tp.min_negative_learning_rate,
                                                                 one_min_completed_train_fraction));

    for (std::size_t i = 0; i < tp.neg_examples; ++i)
    {
      // get next data pair
      dataloader.next_negative(input_word_idx, context_word_idx);

      // forward pass on the model
      model.ForwardAndLoss(input_word_idx, context_word_idx, gt, loss);

      // backward pass
      model.Backward(input_word_idx, context_word_idx, gt);
      ++step_count;
      ++total_step_count;

      sum_loss += (loss * model.alpha_);
    }

    ////////////////////////
    /// IncrementCursors ///
    ////////////////////////

    ++cursor_idx;
    dataloader.IncrementCursors();

    /////////////////////////
    /// print performance ///
    /////////////////////////

    if (cursor_idx % tp.print_freq == 0)
    {
      auto t2   = std::chrono::high_resolution_clock::now();
      time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

      std::cout << "words/sec: " << double(step_count) / time_diff.count() << std::endl;
      t1         = std::chrono::high_resolution_clock::now();
      step_count = 0;

      std::cout << "total_step_count: " << total_step_count << std::endl;
      std::cout << "current cursor idx: " << cursor_idx << std::endl;
      std::cout << "current learning rate: "
                << (tp.learning_rate *
                    fetch::math::Max(tp.min_learning_rate,
                                     1.0 - (double(total_step_count) / tp.total_words)))
                << std::endl;
      std::cout << "loss: " << sum_loss << std::endl;
      sum_loss = 0;

      std::cout << std::endl;

      EvalAnalogy(dataloader, model);
    }
  }

  return 0;
}
