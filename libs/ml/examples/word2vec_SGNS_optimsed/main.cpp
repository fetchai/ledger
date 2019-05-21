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

#include <algorithm>
#include <chrono>
#include <iostream>
#include <math/tensor.hpp>
#include <numeric>

#define TRAINING_WORDS 17000000

// TODO: vocab size should be 71291 - compare vocab parsing
// TODO: implement down sampling frequent words - unigram table
//

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
  SizeType min_word_freq    = 5;      // infrequent word are pruned
  SizeType window_size      = 8;      // one side of the context window

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
  SizeType min_word_freq_;

  std::unordered_map<std::string, SizeType> vocab_;              // unique vocab of words
  std::unordered_map<SizeType, SizeType>    vocab_frequencies_;  // the count of each vocab word

  DataLoader(SizeType max_sentence_len, SizeType min_word_freq, SizeType max_sentences,
             SizeType window_size)
    : data_({max_sentence_len, max_sentences})
    , cursor_offset_(window_size)
    , n_positive_cursors_(2 * window_size)
    , max_sentence_len_(max_sentence_len)
    , min_word_freq_(min_word_freq)
  {
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      positive_cursors_.emplace_back(IteratorType(data_));
    }

    PrepareDynamicWindowProbs();
  }

  void AddData(std::string &text)
  {
    cursor_ = data_.begin();

    // replace all tabs with spaces.
    std::replace(text.begin(), text.end(), '\t', ' ');

    // replace all new lines with spaces.
    std::replace(text.begin(), text.end(), '\n', ' ');

    // replace carriage returns with spaces
    std::replace(text.begin(), text.end(), '\r', ' ');

    vocab_.emplace(std::make_pair("UNK", 0));
    vocab_frequencies_.emplace(std::make_pair(0, 0));

    // initial iteration through words
    SizeType          word_idx;
    std::string       word;
    std::stringstream s(text);
    for (; s >> word;)
    {
      if (!(cursor_.is_valid()))
      {
        break;
      }

      // if already seen this word
      auto it = vocab_.find(word);
      if (it != vocab_.end())
      {
        vocab_frequencies_.at(it->second)++;
        word_idx = it->second;
      }
      else
      {
        word_idx = vocab_.size();
        vocab_frequencies_.emplace(std::make_pair(vocab_.size(), SizeType(1)));
        vocab_.emplace(std::make_pair(word, vocab_.size()));
      }

      // write the word index to the data tensor
      *cursor_ = word_idx;
      ++cursor_;
    }

    // prune infrequent words
    PruneVocab();

    // second iteration to assign data values after pruning
    s                     = std::stringstream(text);
    cursor_               = data_.begin();
    SizeType cursor_count = 0;
    for (; s >> word;)
    {
      if (!(cursor_.is_valid()))
      {
        break;
      }

      // if the word is in the vocab assign it, otherwise assign 0
      auto it = vocab_.find(word);
      if (it != vocab_.end())
      {
        *cursor_ = it->second;
      }
      else
      {
        *cursor_ = 0;
      }
      ++cursor_;
      ++cursor_count;
    }

    // guarantee that data_ is filled with zeroes after the last word added
    for (std::size_t i = cursor_count; i < data_.size(); ++i)
    {
      assert(data_[i] == 0);
    }

    // reset the cursor
    reset_cursor();

    // build the unigram table for negative sampling
    BuildUnigramTable();
  }

  // prune words that are infrequent & sort the words
  void PruneVocab()
  {
    // copy existing vocabs into temporary storage
    std::unordered_map<std::string, SizeType> tmp_vocab              = vocab_;
    std::unordered_map<SizeType, SizeType>    tmp_vocab_frequencies_ = vocab_frequencies_;

    // clear existing vocabs
    vocab_.clear();
    vocab_frequencies_.clear();

    vocab_.emplace(std::make_pair("UNK", 0));
    vocab_frequencies_.emplace(std::make_pair(0, 0));

    // Get pruned idxs
    for (auto &word : tmp_vocab)
    {
      auto cur_freq = tmp_vocab_frequencies_.at(word.second);
      if (cur_freq > min_word_freq_)
      {
        // don't prune
        vocab_.emplace(std::make_pair(word.first, vocab_.size()));
        vocab_frequencies_.emplace(std::make_pair(vocab_.size(), cur_freq));
      }
    }
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

    assert(input_idx < vocab_.size());
  }

  void next_negative(SizeType &input_idx, SizeType &context_idx)
  {
    input_idx = static_cast<SizeType>(*cursor_);

    //    // randomly select a negative cursor from all words in vocab
    //    // TODO: strictly we should ensure this is not one of the positive context words
    //    context_idx = static_cast<SizeType>(gen() % vocab_.size());

    // randomly select an index from the unigram table

    auto ran_val = static_cast<SizeType>(gen() % unigram_size_);

    context_idx = unigram_table_[ran_val];

    assert(context_idx < vocab_size());
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

private:
  static constexpr SizeType unigram_size_ = 1000000;
  SizeType                  unigram_table_[unigram_size_];
  double                    unigram_power_ = 0.75;

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

  void BuildUnigramTable()
  {
    vocab_.size();
    double train_words_pow = 0.;
    double d1              = 0.;

    // compute normalizer
    for (SizeType a = 0; a < vocab_.size(); ++a)
    {
      train_words_pow += pow(vocab_frequencies_[a], unigram_power_);
    }

    //
    SizeType word_idx = 0;
    d1                = pow(vocab_frequencies_[word_idx], unigram_power_) / train_words_pow;

    auto vocab_it = vocab_.begin();

    for (SizeType a = 0; a < unigram_size_; a++)
    {
      unigram_table_[a] = vocab_it->second;
      if ((static_cast<double>(a) / static_cast<double>(unigram_size_)) > d1)
      {
        if (vocab_it != vocab_.end())
        {
          ++vocab_it;
        }
        d1 += pow(vocab_frequencies_[vocab_it->second], unigram_power_) / train_words_pow;
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
  ArrayType input_embeddings_;   // embeddings used to encode the input word
  ArrayType output_embeddings_;  // embeddings used to encode the context word

  ArrayType input_grads_;
  ArrayType context_grads_;

  ArrayType input_vector_;
  ArrayType context_vector_;
  ArrayType result_{{1, 1}};
  ArrayType dot_result_{{1, 1}};

  Type alpha_ = 0.2;  // learning rate

  std::vector<Type> l2reg_input_row_sums;
  std::vector<Type> l2reg_output_row_sums;
  Type              l2reg_input_sum  = 0;
  Type              l2reg_output_sum = 0;

  Type l2_lambda = 0.0000001;

  SkipgramModel(SizeType vocab_size, SizeType embeddings_size, Type learning_rate)
    : input_embeddings_({vocab_size, embeddings_size})
    , output_embeddings_({vocab_size, embeddings_size})  // initialized to all 0
    , input_grads_({1, embeddings_size})
    , context_grads_({embeddings_size, 1})
    , alpha_(learning_rate)
    , l2reg_input_row_sums{std::vector<Type>(vocab_size, 0)}
    , l2reg_output_row_sums{std::vector<Type>(vocab_size, 0)}
  {
    // initialise embeddings to values from (-0.5 / embeddings_size) to (0.5 / embeddings_size)
    input_embeddings_.FillUniformRandom();                             // 0 to 1
    fetch::math::Subtract(input_embeddings_, 0.5, input_embeddings_);  // -0.5 to 0.5
    fetch::math::Divide(input_embeddings_, static_cast<Type>(embeddings_size),
                        input_embeddings_);  // -0.5/embeddings_size to 0.5/embeddings_size

    assert(fetch::math::Max(output_embeddings_) == 0);
    assert(fetch::math::Max(input_embeddings_) <= (0.5 / static_cast<Type>(embeddings_size)));
    assert(fetch::math::Min(input_embeddings_) >= (-0.5 / static_cast<Type>(embeddings_size)));

    // initialise the l2 normalisation values
    for (std::size_t i = 0; i < vocab_size; ++i)
    {
      auto in_it  = input_embeddings_.Slice(i).begin();
      auto out_it = output_embeddings_.Slice(i).begin();
      while (in_it.is_valid())
      {
        l2reg_input_row_sums[i] += (*in_it * *in_it);
        l2reg_output_row_sums[i] += (*out_it * *out_it);
        ++in_it;
        ++out_it;
      }
      l2reg_input_sum += l2reg_input_row_sums[i];
      l2reg_output_sum += l2reg_output_row_sums[i];
    }
  }

  void NormaliseEmbeddingRows(SizeType input_row, SizeType context_row)
  {
    if (l2reg_input_sum > 0)
    {
      auto in_it = input_embeddings_.Slice(input_row).begin();
      while (in_it.is_valid())
      {
        *in_it /= l2reg_input_sum;
        ++in_it;
      }
    }
    else
    {
      std::cout << "l2reg_input_sum <= 0: " << std::endl;
      //      throw std::runtime_error("l2reg_input_sum <= 0 !!!!");
    }

    if (l2reg_output_sum > 0)
    {
      auto out_it = output_embeddings_.Slice(context_row).begin();
      while (out_it.is_valid())
      {
        *out_it /= l2reg_output_sum;
        ++out_it;
      }
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
  void ForwardAndLoss(SizeType input_word_idx, SizeType context_word_idx, Type gt, Type &loss,
                      Type &reg_loss)
  {
    assert(input_word_idx < input_embeddings_.shape()[0]);
    assert(context_word_idx < output_embeddings_.shape()[0]);

    // First normalise the embeddings. Since that's expensive, we just normalise the two rows
    // we'll use
    NormaliseEmbeddingRows(input_word_idx, context_word_idx);

    // TODO - remove these copies when math library SFINAE checks for IsIterable instead of taking
    // arrays. embeddings input & context lookup
    input_vector_   = input_embeddings_.Slice(input_word_idx).Copy();
    context_vector_ = output_embeddings_.Slice(context_word_idx).Copy();

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

    reg_loss = (l2_lambda * (l2reg_input_sum + l2reg_output_sum));

    if (std::isnan(loss))
    {
      throw std::runtime_error("loss is nan");
    }
  }

  void Backward(SizeType const &input_word_idx, SizeType const &context_word_idx, Type const &gt)
  {
    assert(input_word_idx < input_embeddings_.shape()[0]);
    assert(context_word_idx < output_embeddings_.shape()[0]);

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
    auto g = (gt - result_[0]) * alpha_;

    //    // calculate dl/d(v_in)
    //    fetch::math::Multiply(context_vector_, g, input_grads_);
    //
    //    // calculate dl/d(v_out)
    //    fetch::math::Multiply(input_vector_, g, context_grads_);

    // apply gradient updates
    l2reg_input_sum -= l2reg_input_row_sums[input_word_idx];
    l2reg_output_sum -= l2reg_output_row_sums[context_word_idx];

    l2reg_input_row_sums[input_word_idx]    = 0;
    l2reg_output_row_sums[context_word_idx] = 0;

    auto input_slice_it = input_embeddings_.Slice(input_word_idx).begin();
    auto input_grads_it = context_vector_.begin();

    while (input_slice_it.is_valid())
    {
      // grad and l2_reg weight decay
      *input_slice_it += (g * *input_grads_it) - (l2_lambda * *input_slice_it);
      //      *input_slice_it += (g * *input_grads_it);

      l2reg_input_row_sums[input_word_idx] += *input_slice_it;

      ++input_slice_it;
      ++input_grads_it;
    }

    // apply gradient updates
    auto context_slice_it = output_embeddings_.Slice(context_word_idx).begin();
    auto context_grads_it = input_vector_.begin();
    while (context_slice_it.is_valid())
    {
      // grad and l2_reg weight decay
      *context_slice_it += (g * *context_grads_it) - (l2_lambda * *input_slice_it);
      //      *context_slice_it += (g * *context_grads_it);

      l2reg_output_row_sums[context_word_idx] += *context_slice_it;

      ++context_slice_it;
      ++context_grads_it;
    }
    l2reg_input_sum += l2reg_input_row_sums[input_word_idx];

    if (l2reg_input_sum < 0)
    {
      std::cout << "something weird is happening here: " << std::endl;
    }

    l2reg_output_sum += l2reg_output_row_sums[context_word_idx];
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
  SizeType k = 5;

  std::string word1 = "italy";
  std::string word2 = "rome";
  std::string word3 = "france";
  std::string word4 = "paris";

  SizeType word1_idx = dl.vocab_lookup(word1);
  SizeType word2_idx = dl.vocab_lookup(word2);
  SizeType word3_idx = dl.vocab_lookup(word3);
  SizeType word4_idx = dl.vocab_lookup(word4);

  std::cout << "italy_idx: " << word1_idx << std::endl;
  std::cout << "rome_idx: " << word2_idx << std::endl;
  std::cout << "france_idx: " << word3_idx << std::endl;
  std::cout << "paris_idx: " << word4_idx << std::endl;

  auto italy_vector  = model.input_embeddings_.Slice(word1_idx).Copy();
  auto rome_vector   = model.input_embeddings_.Slice(word2_idx).Copy();
  auto france_vector = model.input_embeddings_.Slice(word3_idx).Copy();
  auto paris_vector  = model.input_embeddings_.Slice(word4_idx).Copy();

  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output;

  /// Closest word to Italy ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(model.input_embeddings_, italy_vector, k);
  std::cout << "Closest word to Italy: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << dl.VocabLookup(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Closest word to France ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(model.input_embeddings_, france_vector, k);
  std::cout << "Closest word to France: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << dl.VocabLookup(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Closest word to Rome ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(model.input_embeddings_, rome_vector, k);
  std::cout << "Closest word to Rome: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << dl.VocabLookup(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Closest word to Paris ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(model.input_embeddings_, paris_vector, k);
  std::cout << "Closest word to Paris: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << dl.VocabLookup(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Vector Math analogy: Paris - France + Italy should give us rome ///
  // vector math - hopefully target_vector is close to the location of the embedding value for word4
  auto analogy_target_vector_1 = france_vector - paris_vector + italy_vector;
  output = fetch::math::clustering::KNNCosine(model.input_embeddings_, analogy_target_vector_1, k);

  std::cout << "France - Paris + Italy = : " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << dl.VocabLookup(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Vector Math analogy: Paris - France + Italy should give us rome ///
  // vector math - hopefully target_vector is close to the location of the embedding value for word4
  auto analogy_target_vector_2 = paris_vector - france_vector + italy_vector;
  output = fetch::math::clustering::KNNCosine(model.input_embeddings_, analogy_target_vector_2, k);

  std::cout << "Paris - France + Italy = : " << std::endl;
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
  DataLoader<ArrayType> dataloader(tp.max_sentence_len, tp.min_word_freq, tp.max_sentences,
                                   tp.window_size);

  // load text from files as necessary and process text with dataloader
  std::string training_text_string = ReadFile(training_text);
  dataloader.AddData(training_text_string);

  std::cout << "vocab_size: " << dataloader.vocab_size() << std::endl;

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  SkipgramModel<ArrayType> model(dataloader.vocab_size(), tp.embedding_size, tp.learning_rate);
  tp.negative_learning_rate     = tp.learning_rate / tp.neg_examples;
  tp.min_negative_learning_rate = tp.min_learning_rate / tp.neg_examples;

  ///////////////////////////
  /// BEGIN TRAINING LOOP ///
  ///////////////////////////

  std::cout << "begin training: " << std::endl;

  SizeType input_word_idx;
  SizeType context_word_idx;

  SizeType                      step_count{0};
  SizeType                      cursor_idx{0};
  SizeType                      total_step_count{0};
  std::chrono::duration<double> time_diff;

  DataType gt;

  DataType loss        = 0;
  DataType l2loss      = 0;
  DataType sum_loss    = 0;
  DataType sum_l2_loss = 0;

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

    if (static_cast<SizeType>(*dataloader.cursor_) != 0)  // ignore unknown words
    {
      auto one_min_completed_train_fraction =
          1.0 - (((epoch_count + 1) * static_cast<double>(cursor_idx)) / tp.total_words);

      ////////////////////////////////
      /// run one positive example ///
      ////////////////////////////////
      gt = 1;

      // update learning rate once every 10k steps
      if (cursor_idx % 10000 == 0)
      {
        model.alpha_ = (tp.learning_rate *
                        fetch::math::Max(tp.min_learning_rate, one_min_completed_train_fraction));
      }

      // get next data pair
      dataloader.next_positive(input_word_idx, context_word_idx);

      // forward pass on the model & loss calculation bundled together
      model.ForwardAndLoss(input_word_idx, context_word_idx, gt, loss, l2loss);

      // backward pass
      model.Backward(input_word_idx, context_word_idx, gt);
      ++step_count;
      ++total_step_count;

      sum_loss += (loss * model.alpha_);
      sum_l2_loss += (l2loss * model.alpha_);

      ///////////////////////////////
      /// run k negative examples ///
      ///////////////////////////////
      gt = 0;

      // update learning rate once every 10k steps
      if (cursor_idx % 10000 == 0)
      {
        model.alpha_ =
            (tp.negative_learning_rate *
             fetch::math::Max(tp.min_negative_learning_rate, one_min_completed_train_fraction));
      }

      for (std::size_t i = 0; i < tp.neg_examples; ++i)
      {
        // get next data pair
        dataloader.next_negative(input_word_idx, context_word_idx);

        if (context_word_idx == input_word_idx)
          continue;

        // forward pass on the model
        model.ForwardAndLoss(input_word_idx, context_word_idx, gt, loss, l2loss);

        // backward pass
        model.Backward(input_word_idx, context_word_idx, gt);
        ++step_count;
        ++total_step_count;

        sum_loss += (loss * model.alpha_);
        sum_l2_loss += (l2loss * model.alpha_);
      }
    }

    ////////////////////////
    /// IncrementCursors ///
    ////////////////////////
    dataloader.IncrementCursors();
    ++cursor_idx;

    /////////////////////////
    /// print performance ///
    /////////////////////////

    if (total_step_count % tp.print_freq == 0)
    {

      auto t2   = std::chrono::high_resolution_clock::now();
      time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

      std::cout << "words/sec: " << double(step_count) / time_diff.count() << std::endl;
      t1         = std::chrono::high_resolution_clock::now();
      step_count = 0;

      std::cout << "total_step_count: " << total_step_count << std::endl;
      std::cout << "current cursor idx: " << cursor_idx << std::endl;
      std::cout << "current negative learning rate: " << model.alpha_ << std::endl;
      std::cout << "loss: " << (sum_loss + sum_l2_loss) / tp.print_freq << std::endl;
      std::cout << "w2vloss: " << sum_loss / tp.print_freq << std::endl;
      std::cout << "l2 loss: " << sum_l2_loss / tp.print_freq << std::endl;
      sum_loss    = 0;
      sum_l2_loss = 0;
      std::cout << std::endl;

      EvalAnalogy(dataloader, model);
    }
  }

  return 0;
}
