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

#include "core/random.hpp"
#include "math/base_types.hpp"
#include "math/tensor/tensor.hpp"

#include <fstream>
#include <string>

namespace fetch {
namespace math {
namespace utilities {

/**
 * Loads a csv file into an TensorType (a Tensor)
 * The Tensor will have the same number of rows as this file has (minus rows_to_skip) and the same
 * number of columns (minus cols_to_skip) as the file
 * @param filename  name of the file
 * @param cols_to_skip  number of columns to skip
 * @param rows_to_skip  number of rows to skip
 * @return TensorType with data
 */
template <typename TensorType>
TensorType ReadCSV(std::string const &filename, math::SizeType const cols_to_skip = 0,
                   math::SizeType rows_to_skip = 0, bool unsafe_parsing = false)
{
  using DataType = typename TensorType::Type;
  std::ifstream file(filename);
  if (file.fail())
  {
    throw math::exceptions::InvalidFile("ReadCSV cannot open file " + filename);
  }

  std::string           buf;
  char const            delimiter = ',';
  std::string           field_value;
  fetch::math::SizeType row{0};
  fetch::math::SizeType col{0};

  // find number of rows and columns in the file
  while (std::getline(file, buf, '\n'))
  {
    if (row == 0)
    {
      std::stringstream ss(buf);
      while (std::getline(ss, field_value, delimiter))
      {
        ++col;
      }
    }
    ++row;
  }

  // we swap rows and columns because rows are ordinarily data samples, which should be the trailing
  // dimension of our tensor
  TensorType weights({col - cols_to_skip, row - rows_to_skip});

  // read data into weights array
  std::string token;
  file.clear();
  file.seekg(0, std::ios::beg);

  while (rows_to_skip)
  {
    std::getline(file, buf, '\n');
    rows_to_skip--;
  }

  row = 0;
  while (std::getline(file, buf, '\n'))
  {
    col = 0;
    std::stringstream ss(buf);
    for (math::SizeType i = 0; i < cols_to_skip; i++)
    {
      std::getline(ss, field_value, delimiter);
    }
    while (std::getline(ss, field_value, delimiter))
    {
      if (field_value.empty())
      {
        throw std::runtime_error("Empty field in ReadCSV");
      }
      if (unsafe_parsing)
      {
        // Constructing a fixed point from a double is not guaranteed to give the same results on
        // different architectures and so is unsafe. But the fixed point string parsing does not
        // support scientific notation.
        weights(col, row) = fetch::math::AsType<DataType>(std::stod(field_value));
      }
      else
      {
        weights(col, row) = fetch::math::Type<DataType>(field_value);
      }
      ++col;
    }
    ++row;
  }

  return weights;
}
}  // namespace utilities
}  // namespace math
}  // namespace fetch
