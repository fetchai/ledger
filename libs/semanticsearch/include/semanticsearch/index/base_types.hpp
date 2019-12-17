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

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace fetch {
namespace semanticsearch {

using DBIndexType            = uint64_t;  ///< Database index type. Essentially pointer to record.
using SemanticCoordinateType = uint64_t;  ///< Base coordinate type semantic position.
                                          ///  Always internal 0 -> 1

class SemanticPosition
{
public:
  using Container      = std::vector<SemanticCoordinateType>;
  using iterator       = Container::iterator;
  using const_iterator = Container::const_iterator;

  iterator begin()
  {
    return data_.begin();
  }

  iterator end()
  {
    return data_.end();
  }

  const_iterator begin() const
  {
    return data_.begin();
  }

  const_iterator end() const
  {
    return data_.end();
  }
  const_iterator cbegin() const
  {
    return data_.cbegin();
  }

  const_iterator cend() const
  {
    return data_.cend();
  }

  void PushBack(SemanticCoordinateType x)
  {
    data_.push_back(x);
  }

  SemanticCoordinateType operator[](std::size_t const &n) const
  {
    return data_[n];
  }

  SemanticCoordinateType &operator[](std::size_t const &n)
  {
    return data_[n];
  }

  uint64_t rank() const
  {
    return data_.size();
  }

  uint64_t size() const
  {
    return data_.size();
  }

  SemanticPosition operator+(SemanticPosition const &other) const
  {
    if (rank() != other.rank())
    {
      throw std::runtime_error("Mismatching rank.");
    }

    SemanticPosition ret;

    for (uint64_t idx = 0; idx < other.size(); ++idx)
    {
      ret.PushBack(data_[idx] + other[idx]);
    }

    return ret;
  }

  SemanticPosition operator-(SemanticPosition const &other) const
  {
    if (rank() != other.rank())
    {
      throw std::runtime_error("Mismatching rank.");
    }

    SemanticPosition ret;

    for (uint64_t idx = 0; idx < other.size(); ++idx)
    {
      ret.PushBack(data_[idx] - other[idx]);
    }

    return ret;
  }

  void SetModelName(std::string model)
  {
    model_name_ = model;
  }

  std::string model_name() const
  {
    return model_name_;
  }

private:
  Container   data_;
  std::string model_name_{""};
};

using DBIndexSet    = std::set<DBIndexType>;  ///< Set of indices used to return search results.
using DBIndexSetPtr = std::shared_ptr<DBIndexSet>;

}  // namespace semanticsearch
}  // namespace fetch
