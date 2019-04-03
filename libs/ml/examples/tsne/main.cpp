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

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

using namespace fetch::math;
using namespace fetch::ml;

using DataType  = double;
using ArrayType = Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

#include <math/tensor.hpp>
#include <sstream>

/**
 * i.e. Fill tensor matrix with DataType values from file at path
 * @param matrix output matrix to be filled
 * @param path input Path to file with values
 */
void ReadFile(Tensor<DataType> &matrix, std::string const &path)
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
      DataType num;
      in >> num;
      matrix.Set({i, j}, num);
    }
  }
}

int main()
{
  SizeType RANDOM_SEED           = 123456;
  DataType LEARNING_RATE         = 500;  // (seems very high!)
  SizeType MAX_ITERATIONS        = 100;
  DataType PERPLEXITY            = 20;
  SizeType N_DATA_SIZE           = 100;
  SizeType N_INPUT_FEATURE_SIZE  = 3;
  SizeType N_OUTPUT_FEATURE_SIZE = 2;
  DataType INITIAL_MOMENTUM      = 0.5;
  DataType FINAL_MOMENTUM        = 0.8;
  SizeType FINAL_MOMENTUM_STEPS  = 20;

  // high dimensional tensor of dims n_data_points x n_features
  Tensor<DataType> input_matrix({N_DATA_SIZE, N_INPUT_FEATURE_SIZE});

  // Generate easily separable clusters of data
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

  // Initialize TSNE
  TSNE<Tensor<DataType>> tsn(input_matrix, N_OUTPUT_FEATURE_SIZE, PERPLEXITY, RANDOM_SEED);
  tsn.Optimize(LEARNING_RATE, MAX_ITERATIONS, INITIAL_MOMENTUM, FINAL_MOMENTUM,
               FINAL_MOMENTUM_STEPS);
  std::cout << "Result: " << tsn.GetOutputMatrix().ToString() << std::endl;

  return 0;
}
