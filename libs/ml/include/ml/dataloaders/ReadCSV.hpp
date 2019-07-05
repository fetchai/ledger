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

#include <fstream>
#include <vector>

#include "core/random.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * Loads a csv file into an ArrayType (a Tensor)
 * The Tensor will have the same number of rows as this file has (minus rows_to_skip) and the same
 * number of columns (minus cols_to_skip) as the file, unless transpose=true is specified in which
 * case it will be transposed.
 * @param filename  name of the file
 * @param cols_to_skip  number of columns to skip
 * @param rows_to_skip  number of rows to skip
 * @param transpose  whether to transpose the resulting Tensor
 * @return ArrayType with data
 */
template <typename ArrayType>
ArrayType ReadCSV(std::string const &filename, math::SizeType const cols_to_skip = 0,
                  math::SizeType rows_to_skip = 0, bool transpose = false)
{
  using DataType = typename ArrayType::Type;
  std::ifstream file(filename);
  if (file.fail())
  {
    throw std::runtime_error("ReadCSV cannot open file " + filename);
  }

  std::string           buf;
  const char            delimiter = ',';
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

  ArrayType weights({row - rows_to_skip, col - cols_to_skip});

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
      weights(row, col) = static_cast<DataType>(stod(field_value));
      ++col;
    }
    ++row;
  }

  if (transpose)
  {
    weights = weights.Transpose();
  }
  return weights;
}
}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
