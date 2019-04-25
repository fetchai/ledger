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

class UnigramTable
{
public:
  UnigramTable(unsigned int size = 0, std::vector<uint64_t> const &frequencies = {})
  {
    Reset(size, frequencies);
  }

  void Reset(unsigned int size, std::vector<uint64_t> const &frequencies)
  {
    if (size && frequencies.size())
    {
      data_.resize(size);
      double total(0);
      for (auto const &e : frequencies)
      {
        total += std::pow(e, 0.75);
      }
      std::size_t i(0);
      double      n = pow((double)frequencies[i], 0.75) / total;
      for (std::size_t j(0); j < size; ++j)
      {
        data_[j] = i;
        if ((double)j / (double)size > n)
        {
          i++;
          n += pow((double)frequencies[i], 0.75) / total;
        }
      }
      assert(i == frequencies.size() - 1);
    }
  }

  uint64_t Sample() const
  {
    return data_[(uint64_t)rand() % data_.size()];
  }

private:
  std::vector<uint64_t> data_;
};
