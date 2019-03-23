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

#include "ml/dataloaders/word2vec_loaders/basic_textloader.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * additional params only relent for Skipgram models
 */
template <typename T>
struct CBoWTextParams : TextParams<T>
{
public:
  CBoWTextParams()
    : TextParams<T>(true){};
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class CBoWLoader : public BasicTextLoader<T>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  CBoWTextParams<T> p_;

public:
  explicit CBoWLoader(CBoWTextParams<T> p, SizeType seed = 123456789);

  bool AddData(std::string const &training_data) override;

private:
  void     GetData(SizeType idx, ArrayType &ret) override;
  SizeType GetLabel(SizeType idx) override;
};

template <typename T>
CBoWLoader<T>::CBoWLoader(CBoWTextParams<T> p, SizeType seed)
  : BasicTextLoader<T>(p, seed)
  , p_(p)
{

  // sanity checks on parameters
  assert(p_.window_size > 0);
}

/**
 * get a single training pair from a word index
 * @param idx
 * @return
 */
template <typename T>
void CBoWLoader<T>::GetData(SizeType idx, T &ret)
{
  // the data assignment - left window
  SizeType buffer_count = 0;
  for (SizeType i = idx - p_.window_size; i < idx; ++i)
  {
    SizeType sentence_idx = this->word_idx_sentence_idx.at(i);
    SizeType word_idx     = this->GetWordOffsetFromWordIdx(i);
    ret.At(buffer_count)  = DataType(this->data_.at(sentence_idx).at(word_idx));
    ++buffer_count;
  }

  // the data assignment - left window
  for (SizeType i = idx + 1; i < idx + p_.window_size + 1; ++i)
  {
    SizeType sentence_idx = this->word_idx_sentence_idx.at(i);
    SizeType word_idx     = this->GetWordOffsetFromWordIdx(i);
    ret.At(buffer_count)  = DataType(this->data_.at(sentence_idx).at(word_idx));
    ++buffer_count;
  }
}

template <typename T>
typename CBoWLoader<T>::SizeType CBoWLoader<T>::GetLabel(SizeType idx)
{
  SizeType sentence_idx = this->word_idx_sentence_idx.at(idx);
  SizeType word_idx     = this->GetWordOffsetFromWordIdx(idx);
  return CBoWLoader<T>::SizeType(double(DataType(this->data_.at(sentence_idx).at(word_idx))));
}

/**
 * No additional processing for CBOW
 * @tparam T
 */
template <typename T>
bool CBoWLoader<T>::AddData(std::string const &training_data)
{
  // ordinary pre-processing
  return BasicTextLoader<T>::AddData(training_data);
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
