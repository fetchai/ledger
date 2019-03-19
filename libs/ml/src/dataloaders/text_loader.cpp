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

#include "ml/dataloaders/text_loader.hpp"
//
//#include "math/distance/cosine.hpp"
//#include "math/free_functions/standard_functions/abs.hpp"
//#include "ml/ops/embeddings.hpp"
//
//#include <algorithm>  // random_shuffle
//#include <fstream>    // file streaming
//#include <numeric>    // std::iota
//#include <string>
//
//#include <dirent.h>  // may be compatibility issues

namespace fetch {
namespace ml {
namespace dataloaders {

//
//TextLoader::TextLoader(std::string &data, TextParams<ArrayType> const &p, SizeType seed = 123456789)
//  : cursor_(0)
//  , p_(p)
//  , lfg_(seed)
//  , lcg_(seed)
//{
//  data_buffers_.resize(p_.n_data_buffers);
//
//  // set up training dataset
//  AddData(data);
//}
//
/**
 *  Returns the total number of words
 */
virtual TextLoader::SizeType TextLoader::Size() const
{
  return word_count_;
}

///**
// * Returns the total number of unique words in the vocabulary
// * @return
// */
//SizeType TextLoader::VocabSize()
//{
//  return vocab_.size();
//}
//
///**
// * Returns whether we've iterated through all the data
// * @return
// */
//virtual bool TextLoader::IsDone() const
//{
//  return cursor_ >= word_count_;
//}
//
///**
// * resets the cursor for iterating through multiple epochs
// */
//virtual void TextLoader::Reset()
//{
//  cursor_ = 0;
//}
//
///**
// * Gets the data at specified index
// * @param idx
// * @return  returns a pair of output buffer and label (i.e. X & Y)
// */
//virtual std::pair<T, SizeType> TextLoader::GetAtIndex(SizeType idx)
//{
//  T full_buffer(std::vector<SizeType>({1, p_.n_data_buffers}));
//
//  // pull data from multiple data buffers into single output buffer
//  std::vector<SizeType> buffer_idxs = GetData(idx);
//
//  assert(buffer_idxs.size() == (p_.n_data_buffers + 1));  // 1 extra for the label
//
//  SizeType buffer_count = 0;
//  for (SizeType j = 0; j < p_.n_data_buffers; ++j)
//  {
//    SizeType sentence_idx = word_idx_sentence_idx[buffer_idxs.at(buffer_count)];
//    SizeType word_idx     = GetWordOffsetFromWordIdx(buffer_idxs.at(buffer_count));
//    full_buffer.At(j)     = DataType(data_.at(sentence_idx).at(word_idx));
//    ++buffer_count;
//  }
//
//  SizeType label = buffer_idxs.at(p_.n_data_buffers);
//  cursor_++;
//
//  return std::make_pair(full_buffer, label);
//}
//
///**
// * get the next data point in order
// * @return
// */
//virtual std::pair<T, SizeType> TextLoader::GetNext()
//{
//  // loop until we find a non-discarded data point
//  bool not_found = true;
//  while (not_found)
//  {
//    if (cursor_ >= word_count_)
//    {
//      Reset();
//      is_done_ = true;
//    }
//
//    if (p_.discard_frequent)
//    {
//      SizeType sentence_idx = GetSentenceIdxFromWordIdx(cursor_);
//      SizeType word_offset  = GetWordOffsetFromWordIdx(cursor_);
//
//      if (!(discards_.at(sentence_idx).at(word_offset)))
//      {
//        is_done_  = false;
//        not_found = false;
//      }
//      else
//      {
//        ++cursor_;
//      }
//    }
//    else
//    {
//      is_done_  = false;
//      not_found = false;
//    }
//  }
//
//  return GetAtIndex(cursor_);
//}
//
///**
// * gets the next data point at random
// * @return
// */
//std::pair<T, SizeType> TextLoader::GetRandom()
//{
//  // loop until we find a non-discarded data point
//  bool not_found = true;
//  while (not_found)
//  {
//    if (cursor_ > vocab_.size())
//    {
//      Reset();
//      is_done_             = true;
//      new_random_sequence_ = true;
//    }
//
//    SizeType sentence_idx = GetSentenceIdxFromWordIdx(cursor_);
//    SizeType word_offset  = GetWordOffsetFromWordIdx(cursor_);
//    if (p_.discard_frequent)
//    {
//      if (!(discards_.at(sentence_idx).at(word_offset)))
//      {
//        is_done_  = false;
//        not_found = false;
//      }
//      else
//      {
//        ++cursor_;
//      }
//    }
//    else
//    {
//      is_done_  = false;
//      not_found = false;
//    }
//  }
//
//  if (new_random_sequence_)
//  {
//    ran_idx_ = std::vector<SizeType>(Size());
//    std::iota(ran_idx_.begin(), ran_idx_.end(), 0);
//    std::random_shuffle(ran_idx_.begin(), ran_idx_.end());
//    new_random_sequence_ = false;
//    is_done_             = false;
//  }
//
//  return GetAtIndex(ran_idx_.at(cursor_));
//}
//
///**
// * lookup a vocab index for a word
// * @param idx
// * @return
// */
//SizeType TextLoader::VocabLookup(std::string &idx)
//{
//  assert(vocab_[idx].at(0) < vocab_.size());
//  assert(vocab_[idx].at(0) != 0);  // dont currently handle unknowns elegantly
//  return vocab_[idx].at(0);
//}
//
///**
// * lookup the string corresponding to a vocab index - EXPENSIVE
// *
// * @param idx
// * @return
// */
//std::string TextLoader::VocabLookup(SizeType idx)
//{
//  for (auto &e : vocab_)
//  {
//    if (e.second.at(0) == idx)
//    {
//      return e.first;
//    }
//  }
//  return "UNK";
//}
//
///**
// * helper function for getting the Top K most frequent words
// */
//std::vector<std::pair<std::string, SizeType>> TextLoader::BottomKVocab(SizeType k)
//{
//  return FindK(k, false);
//}
//
///**
// * helper function for getting the Top K most frequent words
// */
//std::vector<std::pair<std::string, SizeType>> TextLoader::TopKVocab(SizeType k)
//{
//  return FindK(k, true);
//}
//
//SizeType GetDiscardCount()
//{
//  return discard_count_;
//}
//
///**
// * return a single sentence from the dataset
// * @param word_idx
// * @return
// */
//SizeType TextLoader::GetSentenceIdxFromWordIdx(SizeType word_idx)
//{
//  return word_idx_sentence_idx[word_idx];
//}
//
///**
// * return a single sentence from the dataset
// * @param word_idx
// * @return
// */
//std::vector<SizeType> TextLoader::GetSentenceFromWordIdx(SizeType word_idx)
//{
//  SizeType sentence_idx = word_idx_sentence_idx[word_idx];
//  return data_[sentence_idx];
//}
//
///**
// * return the word offset within sentence from word idx
// */
//SizeType TextLoader::GetWordOffsetFromWordIdx(SizeType word_idx)
//{
//  SizeType word_offset;
//  SizeType sentence_idx = word_idx_sentence_idx[word_idx];
//
//  if (sentence_idx == 0)
//  {
//    word_offset = word_idx;
//  }
//  else
//  {
//    SizeType cur_word_idx = word_idx - 1;
//    SizeType word_idx_for_offset_zero;
//    bool     not_found = true;
//    while (not_found)
//    {
//      if (sentence_idx != word_idx_sentence_idx[cur_word_idx])
//      {
//        // first word in current sentence is therefore
//        word_idx_for_offset_zero = cur_word_idx + 1;
//        word_offset              = word_idx - word_idx_for_offset_zero;
//        not_found                = false;
//      }
//      else
//      {
//        --cur_word_idx;
//      }
//    }
//  }
//  return word_offset;
//}
//
///**
// * Adds text to training data
// * @param training_data
// */
//void TextLoader::AddData(std::string const &training_data)
//{
//  std::string full_training_text;
//  GetTextString(training_data, full_training_text);
//
//  // converts text into training pairs & related preparatory work
//  ProcessTrainingData(full_training_text);
//}
//
///**
// * print k nearest neighbours
// * @param vocab
// * @param embeddings
// * @param word
// * @param k
// */
//std::vector<std::pair<std::string, double>> TextLoader::GetKNN(ArrayType const &  embeddings,
//                                                               std::string const &word,
//                                                               unsigned int       k)
//{
//  ArrayType wordVector = embeddings.Slice(vocab_.at(word).at(0)).Unsqueeze();
//
//  std::vector<std::pair<SizeType, double>> distances;
//  distances.reserve(VocabSize());
//  for (SizeType i(1); i < VocabSize(); ++i)  // Start at 1, 0 is UNK
//  {
//    DataType d = fetch::math::distance::Cosine(wordVector, embeddings.Slice(i).Unsqueeze());
//    distances.emplace_back(i, d);
//  }
//  std::nth_element(distances.begin(), distances.begin() + k, distances.end(),
//                   [](std::pair<SizeType, double> const &a, std::pair<SizeType, double> const &b) {
//                     return a.second < b.second;
//                   });
//
//  std::vector<std::pair<std::string, double>> ret;
//  for (SizeType i(0); i < k; ++i)
//  {
//    ret.emplace_back(std::make_pair(VocabLookup(distances.at(i).first), distances.at(i).second));
//  }
//  return ret;
//}
//
///**
// * Override this method with a more interesting implementation
// * @param training_data
// */
//virtual std::vector<SizeType> TextLoader::GetData(SizeType idx)
//{
//  assert(p_.n_data_buffers == 1);
//  return {idx, 1};
//}
//
///**
// * This method should be overridden with any additional text-pre-processing
// * specific to the problem
// */
//virtual void TextLoader::AdditionalPreProcess()
//{}
//
///**
// * helper for stripping punctuation from words
// * @param word
// */
//void TextLoader::StripPunctuation(std::string &word)
//{
//  std::string result;
//  std::remove_copy_if(word.begin(), word.end(),
//                      std::back_inserter(result),  // Store output
//                      std::ptr_fun<int, int>(&std::ispunct));
//  word = result;
//}
//
///**
// * checks if word contains end of sentence marker
// * @param word a string representing a word with punctuation in tact
// * @return
// */
//bool TextLoader::CheckEndOfSentence(std::string &word)
//{
//  return std::string(".!?").find(word.back()) != std::string::npos;
//}
//
///**
// * returns a vector of filenames of txt files
// * @param dir_name  the directory to scan
// * @return
// */
//std::vector<std::string> TextLoader::GetAllTextFiles(std::string dir_name)
//{
//  std::vector<std::string> ret;
//  DIR *                    d;
//  struct dirent *          ent;
//  char *                   p1;
//  int                      txt_cmp;
//  if ((d = opendir(dir_name.c_str())) != NULL)
//  {
//    while ((ent = readdir(d)) != NULL)
//    {
//      p1 = strtok(NULL, ".");
//      if (p1 != NULL)
//      {
//        txt_cmp = strcmp(p1, "txt");
//        if (txt_cmp == 0)
//        {
//          ret.emplace_back(ent->d_name);
//        }
//      }
//    }
//    closedir(d);
//  }
//  return ret;
//}
//
///**
// * returns the full training text as one string, either gathered from txt files or pass through
// * @param training_data
// * @param full_training_text
// */
//void TextLoader::GetTextString(std::string const &training_data, std::string &full_training_text)
//{
//  std::vector<std::string> file_names = GetAllTextFiles(training_data);
//  if (file_names.size() == 0)
//  {
//    full_training_text = training_data;
//  }
//  else
//  {
//    for (SizeType j = 0; j < file_names.size(); ++j)
//    {
//      std::string   cur_file = training_data + "/" + file_names[j] + ".txt";
//      std::ifstream t(cur_file);
//
//      std::string cur_text((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
//
//      full_training_text += cur_text;
//    }
//  }
//}
//
///**
// * Preprocesses all training data, then builds vocabulary
// * @param training_data
// */
//void TextLoader::ProcessTrainingData(std::string &training_data)
//{
//  std::vector<std::vector<std::string>> sentences;
//
//  // strip punctuation and handle casing
//  PreProcessWords(training_data, sentences);
//
//  // build unique vocabulary and get word counts
//  BuildVocab(sentences);
//
//  // mark the discard words randomly according to word frequency
//  DiscardFrequent(sentences);
//
//  // inherited data loaders may complete further pre-processing
//  AdditionalPreProcess();
//}
//
///**
// * removes punctuation and handles casing of all words in training data
// * @param training_data
// */
//void TextLoader::PreProcessWords(std::string &                          training_data,
//                                 std::vector<std::vector<std::string>> &sentences)
//{
//  std::string word;
//  SizeType    sentence_count = 0;
//  sentences.push_back(std::vector<std::string>{});
//  for (std::stringstream s(training_data); s >> word;)
//  {
//    if (sentence_count_ + sentence_count > p_.max_sentences)
//    {
//      break;
//    }
//
//    // must check this before we strip punctuation
//    bool new_sentence = CheckEndOfSentence(word);
//
//    // strip punctuation
//    StripPunctuation(word);
//
//    // lower case
//    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
//
//    sentences[sentence_count].push_back(word);
//
//    // if new sentence
//    if (new_sentence)
//    {
//      sentences.push_back(std::vector<std::string>{});
//      ++sentence_count;
//    }
//  }
//
//  // just in case the final word has a full stop or newline - we remove the empty vector
//  if (sentences.back().size() == 0)
//  {
//    sentences.pop_back();
//  }
//}
//
///**
// * builds vocab out of parsed training data
// */
//void TextLoader::BuildVocab(std::vector<std::vector<std::string>> &sentences)
//{
//  // insert words uniquely into the vocabulary
//  vocab_.emplace(std::make_pair("UNK", std::vector<SizeType>({0, 0})));
//  for (std::vector<std::string> &cur_sentence : sentences)
//  {
//    if ((cur_sentence.size() > p_.min_sentence_length) && (sentence_count_ < p_.max_sentences))
//    {
//      data_.push_back(std::vector<SizeType>({}));
//      for (std::string cur_word : cur_sentence)
//      {
//        // if already seen this word
//        SizeType word_idx;
//        if (vocab_.find(cur_word) != vocab_.end())
//        {
//          word_idx           = vocab_[cur_word].at(0);
//          SizeType word_freq = vocab_[cur_word].at(1);
//          ++word_freq;
//
//          vocab_[cur_word] = std::vector<SizeType>({word_idx, word_freq});
//        }
//        else
//        {
//          vocab_.emplace(std::make_pair(cur_word, std::vector<SizeType>({unique_word_count_, 1})));
//          word_idx = unique_word_count_;
//          unique_word_count_++;
//        }
//        data_.at(sentence_count_).push_back(word_idx);
//        word_idx_sentence_idx.emplace(std::pair<SizeType, SizeType>(word_count_, sentence_count_));
//        sentence_idx_word_idx.emplace(std::pair<SizeType, SizeType>(sentence_count_, word_count_));
//        word_count_++;
//      }
//      sentence_count_++;
//    }
//  }
//}
//
///**
// * discards words in training data set based on word frequency
// */
//void TextLoader::DiscardFrequent(std::vector<std::vector<std::string>> &sentences)
//{
//  SizeType sentence_count = 0;
//  // iterate through all sentences
//  for (SizeType sntce_idx = 0; sntce_idx < sentences.size(); sntce_idx++)
//  {
//    if ((sentences[sntce_idx].size() > p_.min_sentence_length) &&
//        (discard_sentence_idx_ + sentence_count < p_.max_sentences))
//    {
//      discards_.push_back({});
//      // iterate through words in the sentence choosing which to discard
//      for (SizeType i = 0; i < sentences[sntce_idx].size(); i++)
//      {
//        if (DiscardExample(sentences[sntce_idx][i]))
//        {
//          discards_.at(discard_sentence_idx_ + sentence_count).push_back(1);
//          ++discard_count_;
//        }
//        else
//        {
//          discards_.at(discard_sentence_idx_ + sentence_count).push_back(0);
//        }
//      }
//
//      ++sentence_count;
//    }
//  }
//  discard_sentence_idx_ += sentence_count;
//}
//
///**
// * according to mikolov et al. we should throw away examples with proportion to how common the
// * word is
// */
//bool TextLoader::DiscardExample(std::string &word)
//{
//  // check word actually present
//  assert(p_.discard_threshold > 0);
//  assert(vocab_.find(word) != vocab_.end());
//  SizeType word_frequency = vocab_.at(word).at(1);
//  assert(word_frequency > 0);
//
//  double word_probability = double(word_frequency) / double(word_count_);
//
//  double prob_thresh = (std::sqrt(word_probability / p_.discard_threshold) + 1.0);
//  prob_thresh *= (p_.discard_threshold / word_probability);
//  double f = lfg_.AsDouble();
//
//  if (f < prob_thresh)
//  {
//    return false;
//  }
//  return true;
//}
//
///**
// * finds the bottom or top K words by frequency
// * @param k number of words to find
// * @param mode true for topK, false for bottomK
// */
//std::vector<std::pair<std::string, SizeType>> TextLoader::FindK(SizeType k, bool mode)
//{
//  std::vector<std::pair<std::string, std::vector<SizeType>>> top_k(k);
//
//  if (mode)
//  {
//    std::partial_sort_copy(vocab_.begin(), vocab_.end(), top_k.begin(), top_k.end(),
//                           [](std::pair<const std::string, std::vector<SizeType>> const &l,
//                              std::pair<const std::string, std::vector<SizeType>> const &r) {
//                             return l.second.at(1) > r.second.at(1);
//                           });
//  }
//  else
//  {
//    std::partial_sort_copy(vocab_.begin(), vocab_.end(), top_k.begin(), top_k.end(),
//                           [](std::pair<const std::string, std::vector<SizeType>> const &l,
//                              std::pair<const std::string, std::vector<SizeType>> const &r) {
//                             return l.second.at(1) < r.second.at(1);
//                           });
//  }
//
//  std::vector<std::pair<std::string, SizeType>> ret(k);
//  SizeType                                      tmp = 0;
//  for (auto &e : ret)
//  {
//    e = std::make_pair(top_k.at(tmp).first, top_k.at(tmp).second.at(1));
//    ++tmp;
//  }
//
//  return ret;
//}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch