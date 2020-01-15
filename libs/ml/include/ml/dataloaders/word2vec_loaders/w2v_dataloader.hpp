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

#include "ml/dataloaders/word2vec_loaders/w2v_dataloader.hpp"

#include "core/random/lcg.hpp"
#include "math/tensor/tensor.hpp"
#include "meta/type_traits.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/word2vec_loaders/unigram_table.hpp"
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

#include <string>
#include <utility>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename T>
class W2VLoader : public DataLoader<fetch::math::Tensor<T>>
{
public:
  static_assert(fetch::meta::IsFloat<T> || math::meta::IsFixedPoint<T>,
                "The intended T is the typename for the data input to the neural network, which "
                "should be a float or double or fixed-point type.");

  static const T WindowContextUnused;

  using TensorType = fetch::math::Tensor<T>;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;
  using VocabType  = Vocab;
  using ReturnType = std::pair<TensorType, std::vector<TensorType>>;

  W2VLoader(SizeType window_size, SizeType negative_samples);

  bool IsDone() const override;
  void Reset() override;
  void SetTestRatio(DataType new_test_ratio) override;
  void SetValidationRatio(DataType new_validation_ratio) override;

  void       RemoveInfrequent(SizeType min);
  void       InitUnigramTable();
  void       GetNext(ReturnType &ret);
  ReturnType GetNext() override;

  bool AddData(std::vector<TensorType> const &input, TensorType const &label) override;

  bool BuildVocab(std::string const &s);
  void SaveVocab(std::string const &filename);
  void LoadVocab(std::string const &filename);
  bool IsModeAvailable(DataLoaderMode mode) override;

  /// accessors and helper functions ///
  SizeType         Size() const override;
  SizeType         vocab_size() const;
  VocabType const &vocab() const;
  std::string      WordFromIndex(SizeType index) const;
  SizeType         IndexFromWord(std::string const &word) const;
  SizeType         window_size();

  LoaderType LoaderCode() override
  {
    return LoaderType::W2V;
  }

private:
  SizeType                                   current_sentence_;
  SizeType                                   current_word_;
  SizeType                                   window_size_;
  SizeType                                   negative_samples_;
  VocabType                                  vocab_;
  std::vector<std::vector<SizeType>>         data_;
  fetch::random::LinearCongruentialGenerator rng_;
  UnigramTable                               unigram_table_;
  DataLoaderMode                             mode_;

  fetch::math::Tensor<T> target_;  // reusable target tensor
  fetch::math::Tensor<T> label_;   // reusable label tensor

  std::vector<SizeType>    StringsToIndices(std::vector<std::string> const &strings);
  std::vector<std::string> PreprocessString(std::string const &s);
  void                     UpdateCursor() override;
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
