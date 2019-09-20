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

#include <utility>

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
#include "core/byte_array/const_byte_array.hpp"
#include "core/digest.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

#include <vector>

namespace fetch {
namespace ml {
namespace distributed_learning {

struct Translator
{
  //  using TensorType = fetch::math::Tensor<float>;  // todo: temporary

  template <typename TensorType>
  TensorType Translate(TensorType gradient_update, const byte_array::ConstByteArray &vocab_hash)
  {
    if (vocab_hash == my_vocab_hash)
    {
      // no need to translate!
      return gradient_update;
    }
    assert(VocabKnown(vocab_hash));

    std::vector<std::string> other_vocab = known_vocabs[vocab_hash];
    TensorType trans_gu = TensorType({my_vocab->reverse_vocab.size(), gradient_update.shape()[1]});
    for (std::size_t i = 0; i < other_vocab.size(); i++)
    {
      std::string word    = other_vocab[i];
      std::size_t trans_i = my_vocab->IndexFromWord(word);
      if (trans_i != dataloaders::Vocab::UNKNOWN_WORD)
      {
        trans_gu.At(trans_i) = gradient_update.At(i);  // todo: check tensor slice syntax
      }
    }
    return trans_gu;
  }

  void SetMyVocab(std::shared_ptr<dataloaders::Vocab> vocab_ptr)
  {
    my_vocab      = std::move(vocab_ptr);
    my_vocab_hash = my_vocab->GetVocabHash();
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
  fetch::DigestMap<std::vector<std::string>> known_vocabs{};
  byte_array::ConstByteArray                 my_vocab_hash;
  std::shared_ptr<dataloaders::Vocab>        my_vocab;
};
}  // namespace distributed_learning
}  // namespace ml
}  // namespace fetch
