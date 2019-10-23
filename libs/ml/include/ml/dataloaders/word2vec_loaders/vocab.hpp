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
#include "core/serializers/group_definitions.hpp"
#include "crypto/sha256.hpp"
#include "math/base_types.hpp"

#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

class Vocab
{
public:
  using SizeType                         = fetch::math::SizeType;
  using DataType                         = std::map<std::string, std::pair<SizeType, SizeType>>;
  using ReverseDataType                  = std::map<SizeType, std::pair<std::string, SizeType>>;
  static constexpr SizeType UNKNOWN_WORD = fetch::math::numeric_max<SizeType>();

  Vocab() = default;

  std::map<SizeType, SizeType> RemoveInfrequentWord(SizeType min);

  void Save(std::string const &filename) const;

  void Load(std::string const &filename);

  std::string WordFromIndex(SizeType index) const;

  SizeType IndexFromWord(std::string const &word) const;

  std::vector<SizeType> PutSentenceInVocab(std::vector<std::string> const &sentence);

  void RemoveSentenceFromVocab(std::vector<SizeType> const &sentence);

  std::vector<SizeType>      GetCounts();
  std::vector<std::string>   GetReverseVocab();
  SizeType                   GetWordCount();
  SizeType                   GetVocabCount() const;
  byte_array::ConstByteArray GetVocabHash();

  template <typename X, typename D>
  friend struct fetch::serializers::MapSerializer;

private:
  void                            SetVocabHash();
  SizeType                        total_count = 0;
  std::map<std::string, SizeType> vocab;          // word -> id
  std::vector<std::string>        reverse_vocab;  // id -> word
  std::vector<SizeType>           counts;         // id -> count
  byte_array::ConstByteArray      vocab_hash;
};

}  // namespace dataloaders
}  // namespace ml

namespace serializers {

template <typename D>
struct MapSerializer<fetch::ml::dataloaders::Vocab, D>
{
public:
  using Type       = fetch::ml::dataloaders::Vocab;
  using DriverType = D;

  static uint8_t const TOTAL_COUNT   = 1;
  static uint8_t const VOCAB         = 2;
  static uint8_t const REVERSE_VOCAB = 3;
  static uint8_t const COUNTS        = 4;
  static uint8_t const VOCAB_HASH    = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &vocab)
  {
    auto map = map_constructor(5);
    map.Append(TOTAL_COUNT, vocab.total_count);
    map.Append(VOCAB, vocab.vocab);
    map.Append(REVERSE_VOCAB, vocab.reverse_vocab);
    map.Append(COUNTS, vocab.counts);
    map.Append(VOCAB_HASH, vocab.vocab_hash);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &vocab)
  {
    map.ExpectKeyGetValue(TOTAL_COUNT, vocab.total_count);
    map.ExpectKeyGetValue(VOCAB, vocab.vocab);
    map.ExpectKeyGetValue(REVERSE_VOCAB, vocab.reverse_vocab);
    map.ExpectKeyGetValue(COUNTS, vocab.counts);
    map.ExpectKeyGetValue(VOCAB_HASH, vocab.vocab_hash);
  }
};

}  // namespace serializers
}  // namespace fetch
