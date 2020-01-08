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

#include "ml/dataloaders/word2vec_loaders/vocab.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <sstream>

namespace fetch {
namespace ml {
namespace dataloaders {

std::vector<Vocab::SizeType> Vocab::PutSentenceInVocab(std::vector<std::string> const &sentence)
{
  std::vector<SizeType> indices;
  indices.reserve(sentence.size());

  for (std::string const &word : sentence)
  {
    auto word_it = vocab.find(word);

    if (word_it != vocab.end())
    {
      counts[word_it->second]++;
      indices.push_back(word_it->second);
    }
    else
    {
      SizeType word_id = vocab.size();
      vocab[word]      = word_id;
      if (word_id >= reverse_vocab.size())
      {
        reverse_vocab.resize(word_id + 128, "");
      }
      if (word_id >= counts.size())
      {
        counts.resize(word_id + 128, SizeType{0});
      }
      reverse_vocab[word_id] = word;

      counts[word_id]++;

      indices.push_back(word_id);
    }
    total_count++;
  }

  reverse_vocab.resize(vocab.size());
  counts.resize(vocab.size());
  SetVocabHash();

  return indices;
}

void Vocab::RemoveSentenceFromVocab(std::vector<Vocab::SizeType> const &sentence)
{
  for (SizeType const &word_id : sentence)
  {
    counts[word_id]--;
    total_count--;
  }
}

/**
 * remove word that have fewer counts then min
 */
std::map<Vocab::SizeType, Vocab::SizeType> Vocab::RemoveInfrequentWord(Vocab::SizeType min)
{

  std::map<SizeType, SizeType> old2new;  // store the old to new relation for futher use
  SizeType                     new_word_id = 0;

  SizeType original_vocab_size = vocab.size();
  for (SizeType word_id = 0; word_id < original_vocab_size; word_id++)
  {
    std::string word = reverse_vocab[word_id];
    if (counts[word_id] < min)
    {
      vocab.erase(word);
    }
    else
    {
      old2new[word_id]           = new_word_id;
      vocab[word]                = new_word_id;
      counts[new_word_id]        = counts[word_id];
      reverse_vocab[new_word_id] = reverse_vocab[word_id];
      total_count -= counts[word_id];
      new_word_id++;
    }
  }

  reverse_vocab.resize(vocab.size());
  counts.resize(vocab.size());
  SetVocabHash();

  return old2new;
}

/**
 * save vocabulary to a file
 * @return
 */
void Vocab::Save(std::string const &filename) const
{
  std::ofstream outfile(filename, std::ios::binary);
  if (outfile.fail())
  {
    throw ml::exceptions::InvalidFile("Cannot open file " + filename);
  }

  outfile << vocab.size() << "\n";
  outfile << total_count << "\n";

  for (auto &val : vocab)
  {
    outfile << val.first << " ";
    outfile << val.second << " ";
    outfile << counts[val.second] << "\n";
  }
}

/**
 * load vocabulary to a file
 * @return
 */
void Vocab::Load(std::string const &filename)
{
  vocab.clear();
  total_count = 0;
  reverse_vocab.clear();
  counts.clear();

  std::ifstream infile(filename, std::ios::binary);
  if (infile.fail())
  {
    throw ml::exceptions::InvalidFile("Cannot open file " + filename);
  }

  SizeType vocab_size;
  infile >> vocab_size;
  infile >> total_count;
  reverse_vocab.resize(vocab_size, "");
  counts.resize(vocab_size, 0);

  std::string word;
  SizeType    idx;
  SizeType    count;

  std::string buf;
  while (std::getline(infile, buf, '\n'))
  {
    if (!buf.empty())
    {
      std::stringstream ss(buf);

      ss >> word >> idx >> count;

      vocab[word]        = idx;
      reverse_vocab[idx] = word;
      counts[idx]        = count;
    }
  }
  SetVocabHash();
}

/**
 * helper method for retrieving a word given its index in vocabulary
 * @tparam T
 * @param index
 * @return
 */
std::string Vocab::WordFromIndex(SizeType index) const
{
  if (reverse_vocab.size() > index)
  {
    return reverse_vocab[index];
  }

  return "";
}

/**
 * helper method for retrieving word index given a word
 * @tparam T
 * @param word
 * @return
 */
math::SizeType Vocab::IndexFromWord(std::string const &word) const
{
  auto word_it = vocab.find(word);
  if (word_it != vocab.end())
  {
    return word_it->second;
  }
  return UNKNOWN_WORD;
}

void Vocab::SetVocabHash()
{
  crypto::SHA256 hasher{};
  for (auto const &item : reverse_vocab)
  {
    hasher.Update(item);
  }
  vocab_hash = hasher.Final();
}

std::vector<Vocab::SizeType> Vocab::GetCounts()
{
  return counts;
}

Vocab::SizeType Vocab::GetWordCount()
{
  return total_count;
}

Vocab::SizeType Vocab::GetVocabCount() const
{
  return reverse_vocab.size();
}

byte_array::ConstByteArray Vocab::GetVocabHash()
{
  return vocab_hash;
}

std::vector<std::string> Vocab::GetReverseVocab()
{
  return reverse_vocab;
}

bool Vocab::WordKnown(std::string const &word) const
{
  return IndexFromWord(word) != Vocab::UNKNOWN_WORD;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
