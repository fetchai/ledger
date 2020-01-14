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

#include "ml/dataloaders/word2vec_loaders/unigram_table.hpp"

namespace fetch {
namespace ml {

/**
 * reset the unigram frequency, and the sampling pool
 * @param count
 * @param size
 */
void UnigramTable::ResetTable(std::vector<SizeType> const &count, SizeType size)
{
  if ((size != 0u) && !count.empty())
  {
    data_.resize(size);

    double total(0);
    for (auto const &c : count)
    {
      total += std::pow(static_cast<double>(c), 0.75);
    }

    std::size_t i(0);
    double      n = pow(static_cast<double>(count[i]), 0.75) / total;
    for (std::size_t j(0); j < size; ++j)
    {
      while (static_cast<double>(j) / static_cast<double>(size) > static_cast<double>(n))
      {
        i++;
        assert(i < count.size());
        n += pow(static_cast<double>(count[i]), 0.75) / total;
      }
      data_[j] = i;
    }
  }
}

/**
 * Returns the unigram table. Useful for testing.
 * @return
 */
std::vector<typename UnigramTable::SizeType> UnigramTable::GetTable()
{
  return data_;
}

/**
 * constructor specifies table size and vector of frequencies to prefill
 * @param size size of the table
 * @param frequencies vector of frequencies used to calculate how many rows per index
 */
UnigramTable::UnigramTable(std::vector<SizeType> const &frequencies, SizeType size)
{
  ResetTable(frequencies, size);
}

/**
 * samples a random data point from the unigram table
 * @return
 */
bool UnigramTable::Sample(SizeType &ret)
{
  ret = data_[rng_() % data_.size()];
  return true;
}

/**
 * samples a negative value from unigram table, method is unsafe (could loop forever)
 * @param positive_index
 * @return
 */
bool UnigramTable::SampleNegative(SizeType positive_index, SizeType &ret)
{
  ret = data_[rng_() % data_.size()];

  SizeType attempt_count = 0;
  while (ret == positive_index)
  {
    ret = data_[rng_() % data_.size()];
    attempt_count++;
    if (attempt_count > timeout_)
    {
      return false;
    }
  }
  return true;
}

/**
 * Sample the negative words based on proposed best probability distribution from original paper
 * @param positive_index
 * @param ret
 * @return
 */
bool UnigramTable::SampleNegative(fetch::math::Tensor<SizeType> const &positive_indices,
                                  SizeType &                           ret)
{
  ret = data_[rng_() % data_.size()];

  SizeType attempt_count = 0;
  while (true)
  {
    bool in_positive_indices = false;
    for (auto i : positive_indices)
    {
      if (ret == i)
      {
        in_positive_indices = true;
        break;
      }
    }
    if (!in_positive_indices)
    {  // if ret is valid, stop searching
      return true;
    }
    ret = data_[rng_() % data_.size()];
    attempt_count++;
    if (attempt_count > timeout_)
    {
      return false;
    }
  }
}

/**
 * resets random number generation for sampling
 */
void UnigramTable::ResetRNG()
{
  rng_.Seed(42 * 1337);
}

}  // namespace ml
}  // namespace fetch
