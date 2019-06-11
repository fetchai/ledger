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


#include "math/tensor.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/state_dict.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"


#include <string>
#include <fstream>
#include <iostream>
#include <ml/ops/transpose.hpp>


using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::math;


ArrayType read_csv(std::string filename, SizeType cols_to_skip = 0, SizeType rows_to_skip = 0, bool transpose = false)
{
    std::ifstream       file(filename);
    std::string buf;

    // find number of rows and columns in the file
    std::string delimiter = ",";
    ulong pos;
    fetch::math::SizeType row{0};
    fetch::math::SizeType col{0};
    while (std::getline(file, buf, '\n'))
    {
        while (row == 0 && ((pos = buf.find(delimiter)) != std::string::npos))
        {
            buf.erase(0, pos + delimiter.length());
            ++col;
        }
        ++row;
    }

    ArrayType weights({row - rows_to_skip, col + 1 - cols_to_skip});

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
        for(SizeType i=0; i < cols_to_skip; i++)
        {
            pos = buf.find(delimiter);
            buf.erase(0, pos + delimiter.length());
        }
        while ((pos = buf.find(delimiter)) != std::string::npos)
        {
            weights(row, col) = std::stof(buf.substr(0, pos));
            buf.erase(0, pos + delimiter.length());
            ++col;
        }
        weights(row, col) = std::stof(buf);
        ++row;
    }

    if (transpose)
    {
        weights = weights.Transpose();
    }
    return weights;
}


int main()
{

    /// DEFINE NEURAL NET ARCHITECTURE ///

    fetch::ml::Graph<ArrayType>       g;
    DataType dropout_prob0 = 1.0f;  // this is the probability that an input is *kept* and not dropped
    DataType dropout_prob1 = 1.0f;  // set to 1.0 for testing, 0.6 and 0.8 for training

    // extract weights for each
    ArrayType weights1 = read_csv("/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/hidden_dense_1/hidden_dense_1_12/kernel:0.csv",
                                  0, 0, true);
    ArrayType bias1 = read_csv(   "/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/hidden_dense_1/hidden_dense_1_12/bias:0.csv",
                                  0, 0, false);
    ArrayType weights2 = read_csv("/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/hidden_dense_2/hidden_dense_2_4/kernel:0.csv",
                                  0, 0, true);
    ArrayType bias2 = read_csv(   "/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/hidden_dense_2/hidden_dense_2_4/bias:0.csv",
                                  0, 0, false);
    ArrayType weights3 = read_csv("/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/output_dense/output_dense_12/kernel:0.csv",
                                  0, 0, true);
    ArrayType bias3 = read_csv(   "/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/output_dense/output_dense_12/bias:0.csv",
                                  0, 0, false);

    assert(bias1.shape()[0] == weights1.shape()[0]);

    std::string input    = g.AddNode<PlaceHolder<ArrayType>>("Input", {});
    std::string dropout0 = g.AddNode<Dropout<ArrayType>>("dropout0", {input}, dropout_prob0);
    std::string fc1      = g.AddNode<FullyConnected<ArrayType>>("fc1", {dropout0}, 118u, 216u, fetch::ml::details::ActivationType::SOFTMAX);
    std::string dropout1 = g.AddNode<Dropout<ArrayType>>("dropout2", {fc1}, dropout_prob1);
    std::string fc2      = g.AddNode<FullyConnected<ArrayType>>("fc2", {dropout1}, 216u, 108u, fetch::ml::details::ActivationType::SOFTMAX);
    std::string dropout2 = g.AddNode<Dropout<ArrayType>>("dropout3", {fc2}, dropout_prob1);
    std::string fc3      = g.AddNode<FullyConnected<ArrayType>>("fc3", {dropout2}, 108u, 54u, fetch::ml::details::ActivationType::SOFTMAX);

    /// LOAD WEIGHTS INTO GRAPH ///
    auto sd = g.StateDict();

    auto fc1_weights = sd.dict_[fc1 + "_FC_Weights"].weights_;
    auto fc1_bias    = sd.dict_[fc1 + "_FC_Bias"].weights_;
    auto fc2_weights = sd.dict_[fc2 + "_FC_Weights"].weights_;
    auto fc2_bias    = sd.dict_[fc2 + "_FC_Bias"].weights_;
    auto fc3_weights = sd.dict_[fc3 + "_FC_Weights"].weights_;
    auto fc3_bias    = sd.dict_[fc3 + "_FC_Bias"].weights_;

    *fc1_weights = weights1;
    *fc1_bias = bias1;
    *fc2_weights = weights2;
    *fc2_bias = bias2;
    *fc3_weights = weights3;
    *fc3_bias = bias3;


    // load state dict into graph (i.e. load pretrained weights)
    g.LoadStateDict(sd);

    /// FORWARD PASS PREDICTIONS ///
    ArrayType test_x = read_csv("/home/emmasmith/Development/best_models/keras_h7_aluminium_px_last_us_x_test.csv", 1, 1);
    ArrayType test_y = read_csv("/home/emmasmith/Development/best_models/keras_h7_aluminium_px_last_us_y_pred_test.csv", 1, 1);

    SizeType test_index = 42;
    auto current_input = test_x.Slice(test_index).Copy().Transpose();
    g.SetInput(input, current_input);


    ArrayType output = g.Evaluate(fc3);
    // output should be the same as test_y
    std::cout << "output: " << output.Transpose().ToString() << std::endl;
    std::cout << "test_y: " << (test_y.Slice(test_index)).Copy().ToString() << std::endl;
}