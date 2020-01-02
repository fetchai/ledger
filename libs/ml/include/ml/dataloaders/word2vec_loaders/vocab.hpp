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

  bool WordKnown(std::string const &word) const;

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
}  // namespace fetch
