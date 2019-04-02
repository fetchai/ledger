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

#include <math/tensor.hpp>

#include "ml/clustering/tsne.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

using namespace fetch::math;
using namespace fetch::math::distance;

using DataType  = double;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

#include <math/tensor.hpp>
#include <sstream>

/**
 * i.e. Fill tensor matrix with DataType values from file at path
 * @param matrix output matrix to be filled
 * @param path input Path to file with values
 */
void ReadFile(fetch::math::Tensor<double> &matrix, std::string const &path)
{

  std::ifstream source;                  // build a read-Stream
  source.open(path, std::ios_base::in);  // open data

  if (!source)
  {  // if it does not work
    std::cerr << "Can't open file: " << path << std::endl;
  }

  for (std::size_t i = 0; i < matrix.shape().at(0); i++)
  {
    std::string line;
    std::getline(source, line);
    std::istringstream in(line);  // make a stream for the line itself

    for (std::size_t j = 0; j < matrix.shape().at(1); j++)
    {
      double num;
      in >> num;
      matrix.Set({i, j}, num);
    }
  }
}

int main()
{
  SizeType n_data_size           = 100;
  SizeType n_input_feature_size  = 3;
  SizeType n_output_feature_size = 2;

  // high dimensional temson of dims n_data_points x n_featrues
  Tensor<double> input_matrix({n_data_size, n_input_feature_size});
  Tensor<double> output_matrix({n_data_size, n_output_feature_size});

  for (SizeType i = 0; i < 25; ++i)
  {
    input_matrix.Set({i, 0}, -static_cast<DataType>(i) - 50);
    input_matrix.Set({i, 1}, -static_cast<DataType>(i) - 50);
    input_matrix.Set({i, 2}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    input_matrix.Set({i, 0}, -static_cast<DataType>(i) - 50);
    input_matrix.Set({i, 1}, static_cast<DataType>(i) + 50);
    input_matrix.Set({i, 2}, static_cast<DataType>(i) + 50);
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    input_matrix.Set({i, 0}, static_cast<DataType>(i) + 50);
    input_matrix.Set({i, 1}, -static_cast<DataType>(i) - 50);
    input_matrix.Set({i, 2}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    input_matrix.Set({i, 0}, static_cast<DataType>(i) + 50);
    input_matrix.Set({i, 1}, static_cast<DataType>(i) + 50);
    input_matrix.Set({i, 2}, static_cast<DataType>(i) + 50);
  }

  fetch::ml::TSNE<Tensor<double>> tsn(input_matrix, n_output_feature_size, 123456);
  tsn.Optimize(500, 100);
  std::cout << "Result: " << tsn.GetOutputMatrix().ToString() << std::endl;

  return 0;
}
