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

#include "math/tensor/tensor.hpp"

#include "core/byte_array/const_byte_array.hpp"

#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/word2vec_loaders/unigram_table.hpp"
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

#include <map>
#include <string>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename TensorType>
class GraphW2VLoader : public DataLoader<TensorType>
{
public:
  using DataType     = typename TensorType::Type;
  using SizeType     = fetch::math::SizeType;
  using VocabType    = Vocab;
  using VocabPtrType = std::shared_ptr<VocabType>;
  using ReturnType   = std::pair<TensorType, std::vector<TensorType>>;

  const DataType BufferPositionUnusedDataType = fetch::math::numeric_max<DataType>();
  const SizeType BufferPositionUnusedSizeType = fetch::math::numeric_max<SizeType>();

  GraphW2VLoader(SizeType window_size, SizeType negative_samples, DataType freq_thresh,
                 SizeType max_word_count, SizeType seed = 1337);

  bool       IsDone() const override;
  void       Reset() override;
  void       RemoveInfrequent(SizeType min);
  void       RemoveInfrequentFromData(SizeType min);
  void       InitUnigramTable(SizeType size = 1e8, bool use_vocab_frequencies = true);
  ReturnType GetNext() override;
  bool       AddData(std::vector<TensorType> const &input, TensorType const &label) override;

  void SetTestRatio(float new_test_ratio) override;
  void SetValidationRatio(float new_validation_ratio) override;

  void     BuildVocabAndData(std::vector<std::string> const &sents, SizeType min_count = 0,
                             bool build_data = true);
  void     BuildData(std::vector<std::string> const &sents, SizeType min_count = 0);
  void     SaveVocab(std::string const &filename);
  void     LoadVocab(std::string const &filename);
  DataType EstimatedSampleNumber();

  bool WordKnown(std::string const &word) const;
  bool IsModeAvailable(DataLoaderMode mode) override;

  /// accessors and helper functions ///
  SizeType            Size() const override;
  SizeType            vocab_size() const;
  VocabPtrType const &GetVocab() const;
  std::string         WordFromIndex(SizeType index) const;
  SizeType            IndexFromWord(std::string const &word) const;
  SizeType            WindowSize();

  byte_array::ConstByteArray GetVocabHash();

  LoaderType LoaderCode() override
  {
    return LoaderType::SGNS;
  }

private:
  SizeType                           current_sentence_;
  SizeType                           current_word_;
  SizeType                           window_size_;
  SizeType                           negative_samples_;
  DataType                           freq_thresh_;
  VocabPtrType                       vocab_ = std::make_shared<VocabType>();
  std::vector<std::vector<SizeType>> data_;
  std::vector<SizeType>              word_id_counts_;
  UnigramTable                       unigram_table_;
  SizeType                           max_word_count_;
  SizeType                           size_        = 0;
  SizeType                           reset_count_ = 0;

  // temporary sample and labels for buffering samples
  TensorType                    input_words_, output_words_, labels_;
  fetch::math::Tensor<SizeType> output_words_buffer_;
  SizeType                      buffer_pos_ = 0;
  ReturnType                    cur_sample_;

  static std::vector<std::string> PreprocessString(std::string const &s, SizeType length_limit);
  void                            BufferNextSamples();
  void                            UpdateCursor() override;
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
