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

#include "core/random/lcg.hpp"
#include "core/random/lfg.hpp"
#include "ml/dataloaders/dataloader.hpp"

#include <algorithm>  // random_shuffle
#include <fstream>    // file streaming
#include <numeric>    // std::iota
#include <string>
#include <unordered_map>
#include <vector>

#include <dirent.h>  // may be compatibility issues

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename T>
struct TextParams
{

public:
  using SizeType = typename T::SizeType;

  TextParams(){};

  SizeType n_data_buffers      = 0;  // number of data points to return when called
  SizeType max_sentences       = 0;  // maximum number of sentences in training set
  SizeType min_sentence_length = 0;  // minimum number of words in a sentence
  SizeType window_size         = 0;  // the size of the context window

  // optional processing
  bool discard_frequent = false;  // discard frequent words

  // discard params
  double discard_threshold = 0.00001;  // random discard probability threshold
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class TextLoader : public DataLoader<T, typename T::SizeType>
{
public:
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

protected:
  // training data parsing containers
  SizeType                                               size_ = 0;  // # training pairs
  std::unordered_map<std::string, std::vector<SizeType>> vocab_;  // full unique vocab + frequencies
  //  std::unordered_map<SizeType, std::string> reverse_vocab_;    // full unique vocab
  //  std::unordered_map<std::string, SizeType> vocab_frequency_;  // word frequencies
  std::vector<double>                adj_vocab_frequency_;  // word frequencies
  std::vector<std::vector<SizeType>> data_;                 // all training data by sentence
  std::vector<std::vector<SizeType>> discards_;             // record of discarded words
  SizeType discard_sentence_idx_ = 0;  // keeps track of sentences already having discard applied

  // used for iterating through all examples incrementally
  SizeType cursor_;                   // indexes through data
  bool     is_done_         = false;  // tracks progress of cursor
  bool new_random_sequence_ = true;   // whether to generate a new random sequence for training data
  std::vector<SizeType> ran_idx_;     // random indices container

  // params
  TextParams<ArrayType> p_;

  // counts
  SizeType sentence_count_    = 0;  // total sentences in training corpus
  SizeType word_count_        = 0;  // total words in training corpus
  SizeType discard_count_     = 0;  // total count of discarded (frequent) words
  SizeType unique_word_count_ = 1;  // 0 reserved for unknown word

  std::unordered_map<SizeType, SizeType>
      word_idx_sentence_idx;  // lookup table for sentence number from word number
  std::unordered_map<SizeType, SizeType>
      sentence_idx_word_idx;  // lookup table for word number from word sentence

  // containers for the data and labels
  std::vector<std::vector<SizeType>> data_buffers_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

public:
  explicit TextLoader(std::string &data, TextParams<ArrayType> const &p, SizeType seed = 123456789);

  virtual SizeType               Size();
  SizeType                       VocabSize();
  virtual bool                   IsDone();
  virtual void                   Reset();
  virtual std::pair<T, SizeType> GetAtIndex(SizeType idx);
  virtual std::pair<T, SizeType> GetNext();
  std::pair<T, SizeType>         GetRandom();
  SizeType                       VocabLookup(std::string &idx);
  std::string                    VocabLookup(SizeType idx);
  SizeType                       GetDiscardCount();

  std::vector<std::pair<std::string, SizeType>> BottomKVocab(SizeType k);
  std::vector<std::pair<std::string, SizeType>> TopKVocab(SizeType k);

  SizeType              GetSentenceIdxFromWordIdx(SizeType word_idx);
  std::vector<SizeType> GetSentenceFromWordIdx(SizeType word_idx);
  SizeType              GetWordOffsetFromWordIdx(SizeType word_idx);

  void AddData(std::string const &training_data);
  std::vector<std::pair<std::string, double>> GetKNN(ArrayType const &  embeddings,
                                                     std::string const &word, unsigned int k) const;

private:
  /////////////////////////////////////////////////////////////////////
  /// THESE METHODS MUST BE OVERWRIDDEN TO INHERIT FROM TEXT LOADER ///
  /////////////////////////////////////////////////////////////////////

  virtual std::vector<SizeType> GetData(SizeType idx);
  virtual void                  AdditionalPreProcess();

  ///////////////////////////////
  /// THESE METHODS NEED NOT  ///
  ///////////////////////////////

  void                     StripPunctuation(std::string &word);
  bool                     CheckEndOfSentence(std::string &word);
  std::vector<std::string> GetAllTextFiles(std::string dir_name);
  void GetTextString(std::string const &training_data, std::string &full_training_text);
  void ProcessTrainingData(std::string &training_data);
  void PreProcessWords(std::string &                          training_data,
                       std::vector<std::vector<std::string>> &sentences);
  void BuildVocab(std::vector<std::vector<std::string>> &sentences);
  void DiscardFrequent(std::vector<std::vector<std::string>> &sentences);
  bool DiscardExample(std::string &word);
  std::vector<std::pair<std::string, SizeType>> FindK(SizeType k, bool mode);

};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
