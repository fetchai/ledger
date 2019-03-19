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

#include "ml/dataloaders/text_loader.hpp"
#include "math/free_functions/standard_functions/abs.hpp"

//
//#include <algorithm>
//#include <exception>
//#include <fstream>
//#include <iostream>
//#include <memory>
//#include <sstream>
//#include <string>
//#include <unordered_map>
//#include <utility>
//#include <vector>
//
//#include <dirent.h>  // may be compatibility issues

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * additional params only relent for Skipgram models
 */
template <typename T>
struct SkipGramTextParams : TextParams<T>
{
public:
  SkipGramTextParams()
    : TextParams<T>(){};

  using SizeType              = typename TextParams<T>::SizeType;
  SizeType k_negative_samples = 0;  // # negative samples per positive training pair

  bool unigram_table = true;  // build a unigram table

  // unigram params
  SizeType unigram_table_size = 10000000;  // size of unigram table for negative sampling
  double   unigram_power      = 0.75;      // adjusted unigram distribution
  SizeType unigram_precision  = 10;
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class SkipGramLoader : public TextLoader<T>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  // training data parsing containers
  SizeType pos_size_ = 0;  // # positive training pairs
  SizeType neg_size_ = 0;  // # negative training pairs

  SkipGramTextParams<T> p_;

  // the unigram table container
  std::vector<SizeType> unigram_table_;

  double positive_threshold_ = 0.0;

public:
  explicit SkipGramLoader(std::string &data, SkipGramTextParams<T> p, SizeType seed = 123456789);

private:

  virtual std::vector<SizeType> GetData(SizeType idx) override;
  virtual void AdditionalPreProcess() override;
  void BuildUnigramTable();
  bool SelectValence();
  std::vector<SizeType> GeneratePositive(SizeType idx);
  std::vector<SizeType> GenerateNegative(SizeType idx);
  SizeType SelectNegativeContextWord(SizeType idx);
  SizeType SelectContextPosition(SizeType idx);
  bool WindowPositionCheck(SizeType target_pos, SizeType context_pos, SizeType sentence_len) const;
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
