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

namespace fetch {
namespace ml {

class Vocab
{
public:
  using SizeType        = fetch::math::SizeType;
  using DataType        = std::map<std::string, std::pair<SizeType, SizeType>>;
  using ReverseDataType = std::map<SizeType, std::pair<std::string, SizeType>>;

  SizeType total_count;

  DataType        data;          // word -> (id, count)
  ReverseDataType reverse_data;  // id -> (word, count)

  Vocab();

  void Update();

  void                         RemoveInfrequentWord(SizeType min);
  std::map<SizeType, SizeType> Compactify();

  ReverseDataType GetReverseVocab();

  void Save(std::string const &filename) const;
  void Load(std::string const &filename);

  std::string WordFromIndex(SizeType index) const;
  SizeType    IndexFromWord(std::string const &word) const;
};

/**
 * reserve numerical max of SizeType for unknown word
 *
 */
Vocab::Vocab()
{}

/**
 * Get the total count of the vocabulary
 */
void Vocab::Update()
{
  // update total count
  total_count = 0;
  for (auto const &w : data)
  {
    total_count += w.second.second;
  }

  // update reverse vocab
  reverse_data = GetReverseVocab();
}

/**
 * remove word that have less counts then min
 */
void Vocab::RemoveInfrequentWord(fetch::ml::Vocab::SizeType min)
{
  for(auto it = data.begin(); it != data.end();){
    if (it->second.second < min)
    {
      it = data.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

/**
 * Compactify the vocabulary such that there is no skip in indexing
 */
std::map<Vocab::SizeType, Vocab::SizeType> Vocab::Compactify()
{
  std::map<SizeType, SizeType> old2new;  // store the old to new relation for futher use
  SizeType                     i = 0;
  for (auto &word : data)
  {
    old2new[word.second.first] = i;
    word.second.first          = i;
    i++;
  }
  return old2new;
}

/**
 * Return a reversed vocabulary
 */
Vocab::ReverseDataType Vocab::GetReverseVocab()
{
  ReverseDataType reverse_data;
  for (auto const &word : data)
  {
    reverse_data[word.second.first] = std::make_pair(word.first, word.second.second);
  }
  return reverse_data;
}

/**
 * save vocabulary to a file
 * @return
 */
void Vocab::Save(std::string const &filename) const
{
  std::ofstream outfile(filename, std::ios::binary);

  for (auto &val : data)
  {
    outfile << val.first << " ";
    outfile << val.second.first << " ";
    outfile << val.second.second << "\n";
  }
}

/**
 * load vocabulary to a file
 * @return
 */
void Vocab::Load(std::string const &filename)
{
  data.clear();

  std::ifstream infile(filename, std::ios::binary);

  std::string line;
  std::string word;
  SizeType    idx;
  SizeType    count;

  std::pair<SizeType, SizeType>                         p1;
  std::pair<std::string, std::pair<SizeType, SizeType>> p2;
  while (infile.peek() != EOF)
  {
    infile >> line;
    word = line;

    infile >> line;
    idx = std::stoull(line);

    infile >> line;
    count = std::stoull(line);

    p1 = {idx, count};
    p2 = {word, p1};
    data.insert(p2);
  }
}

/**
 * helper method for retrieving a word given its index in vocabulary
 * @tparam T
 * @param index
 * @return
 */
std::string Vocab::WordFromIndex(SizeType index) const
{
  for (auto const &kvp : data)
  {
    if (kvp.second.first == index)
    {
      return kvp.first;
    }
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
  if (data.find(word) != data.end())
  {
    return (data.at(word)).first;
  }
  return fetch::math::numeric_max<SizeType>();
}

}  // namespace ml
}  // namespace fetch
