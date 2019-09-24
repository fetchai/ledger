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

#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace distributed_learning {

struct Translator
{
  using SizeType = fetch::math::SizeType;

  template <typename TensorType>
  std::pair<TensorType, TensorType> Translate(TensorType                        gradient_update,
                                              const byte_array::ConstByteArray &vocab_hash)
  {
    using DataType = typename TensorType::Type;

    TensorType mask = TensorType({MyVocabSize()});

    if (vocab_hash == MyVocabHash())  // no need to translate
    {
      mask.Fill(DataType{1});
      return std::make_pair(gradient_update, mask);
    }

    assert(VocabKnown(vocab_hash));

    std::vector<std::string> other_vocab = known_vocabs[vocab_hash];

    // figure out which way around the matrix is
    bool       first_dim_embedding;
    SizeType   embedding_size;
    TensorType transl_gu;

    if (gradient_update.shape()[0] == other_vocab.size())
    {
      first_dim_embedding = false;
      embedding_size      = gradient_update.shape()[1];
      transl_gu           = TensorType({MyVocabSize(), embedding_size});
    }
    else
    {
      first_dim_embedding = true;
      embedding_size      = gradient_update.shape()[0];
      transl_gu           = TensorType({embedding_size, MyVocabSize()});
    }

    for (SizeType i = 0; i < other_vocab.size(); i++)
    {
      std::string word     = other_vocab[i];
      std::size_t transl_i = my_vocab->IndexFromWord(word);

      if (transl_i != dataloaders::Vocab::UNKNOWN_WORD)
      {
        if (first_dim_embedding)
        {
          for (SizeType j = 0; j < embedding_size; j++)
          {
            transl_gu.At(j, transl_i) = gradient_update.At(j, i);
          }
        }
        else
        {
          for (SizeType j = 0; j < embedding_size; j++)
          {
            transl_gu.Slice(transl_i).Assign(gradient_update.Slice(i));
          }
        }
        mask.At(transl_i) += DataType{1};
      }
    }
    return std::make_pair(transl_gu, mask);
  }

  void SetMyVocab(std::shared_ptr<dataloaders::Vocab> vocab_ptr)
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
  fetch::DigestMap<std::vector<std::string>> known_vocabs{};
  std::shared_ptr<dataloaders::Vocab>        my_vocab;
};
}  // namespace distributed_learning
}  // namespace ml
}  // namespace fetch
