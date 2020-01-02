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

#include "core/byte_array/const_byte_array.hpp"
#include "core/digest.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

#include <utility>
#include <vector>

namespace fetch {
namespace dmlf {
namespace collective_learning {

/** Handles translation of weights and gradients from one vocabulary to another
 *
 */
struct Translator
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = std::vector<SizeType>;

  /**
   * Translates vector of updated rows from one vocabulary to another
   * @tparam TensorType
   * @param updated_rows vector of updated rows
   * @param vocab_hash
   * @return
   */
  template <typename TensorType>
  std::vector<SizeType> TranslateUpdate(SizeVector                        updated_rows,
                                        const byte_array::ConstByteArray &vocab_hash)
  {
    if (vocab_hash == MyVocabHash())  // no need to translate
    {
      return updated_rows;
    }

    assert(VocabKnown(vocab_hash));

    std::vector<std::string> other_vocab = known_vocabs[vocab_hash];

    SizeVector translated_gradient_update(updated_rows);

    for (SizeType i{0}; i < updated_rows.size(); i++)
    {
      std::string word                 = other_vocab[updated_rows.at(i)];
      SizeType    translated_index     = my_vocab->IndexFromWord(word);
      translated_gradient_update.at(i) = translated_index;
    }

    return translated_gradient_update;
  }

  /**
   * Translates weights from one vocabulary to another
   * @tparam TensorType
   * @param weights tensor of embedding weights
   * @param vocab_hash
   * @return
   */
  template <typename TensorType>
  std::pair<TensorType, TensorType> TranslateWeights(TensorType                        weights,
                                                     const byte_array::ConstByteArray &vocab_hash)
  {
    using DataType = typename TensorType::Type;

    TensorType mask = TensorType({MyVocabSize()});

    if (vocab_hash == MyVocabHash())  // no need to translate
    {
      mask.Fill(DataType{1});
      return std::make_pair(weights, mask);
    }

    assert(VocabKnown(vocab_hash));

    // Get vocab from database
    std::vector<std::string> other_vocab = known_vocabs[vocab_hash];

    TensorType translated_gradient_update;

    for (SizeType i = 0; i < other_vocab.size(); i++)
    {
      std::string word             = other_vocab[i];
      SizeType    translated_index = my_vocab->IndexFromWord(word);

      if (translated_index != fetch::ml::dataloaders::Vocab::UNKNOWN_WORD)
      {
        for (SizeType j = 0; j < weights.shape().at(1); j++)
        {
          translated_gradient_update.View(translated_index).Assign(weights.View(i));
        }
        mask.At(translated_index) += DataType{1};
      }
    }

    return std::make_pair(translated_gradient_update, mask);
  }

  void SetMyVocab(std::shared_ptr<fetch::ml::dataloaders::Vocab> vocab_ptr)
  {
    my_vocab = std::move(vocab_ptr);
  }

  SizeType MyVocabSize()
  {
    return my_vocab->GetVocabCount();
  }

  byte_array::ConstByteArray MyVocabHash()
  {
    return my_vocab->GetVocabHash();
  }

  void AddVocab(const byte_array::ConstByteArray &vocab_hash, const std::vector<std::string> &vocab)
  {
    known_vocabs[vocab_hash] = vocab;
  }

  bool VocabKnown(const byte_array::ConstByteArray &vocab_hash)
  {
    return known_vocabs.find(vocab_hash) != known_vocabs.end();
  }

private:
  fetch::DigestMap<std::vector<std::string>>     known_vocabs{};
  std::shared_ptr<fetch::ml::dataloaders::Vocab> my_vocab;
};
}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
