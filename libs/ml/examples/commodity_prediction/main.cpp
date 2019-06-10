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
#include "ml/ops/activations/dropout.hpp"
#include <string>
#include <fstream>


using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
#define MAXC  128


void read_csv(std::string filename)
{
    std::ifstream       file(filename);
    std::string buf, buf2;
//    char buf[MAXC];

    std::vector<std::vector<DataType>> weights;

    fetch::math::SizeType idx{0};
    std::string delimiter = ",";
    ulong pos;
    std::string token;

    while (std::getline(file, buf, '\n'))
    {
//        buf.substr(0, buf.find(','));
        weights.emplace_back();
        while ((pos = buf.find(delimiter)) != std::string::npos) {

            token = buf.substr(0, pos);
            std::cout << token << std::endl;
            buf.erase(0, pos + delimiter.length());
            weights.at(idx).emplace_back(std::stof(token));
        }
        weights.at(idx).emplace_back(std::stof(buf));

        idx++;

//        std::stringstream ss(buf);
//        idx = 0;
//        while (std::getline(ss, buf2, ','))
//        {
//            weights[idx] = std::stof(buf2);
//            idx++;
//        }
    }

    std::cout << "useless statement: " << std::endl;


}





//int main(int ac, char **av)
int main()
{
    fetch::ml::Graph<ArrayType>       g;
//    fetch::math::SizeType batchsize = 42;
    DataType dropout_prob = 0.5;

    read_csv("/home/emmasmith/Development/best_models/output/keras_h7_aluminium_px_last_us/model_weights/hidden_dense_1/hidden_dense_1_12/kernel:0.csv");

    std::string input = g.AddNode<PlaceHolder<ArrayType>>("Input", {});
    std::string fc1 = g.AddNode<FullyConnected<ArrayType>>("fc1", {input}, 118u, 216u);
    std::string dropout1 = g.AddNode<Dropout<ArrayType>>("dropout1", {fc1}, dropout_prob);
    std::string fc2 = g.AddNode<FullyConnected<ArrayType>>("fc2", {dropout1}, 216u, 108u);
    std::string dropout2 = g.AddNode<Dropout<ArrayType>>("dropout2", {fc2}, dropout_prob);
    std::string fc3 = g.AddNode<FullyConnected<ArrayType>>("fc3", {dropout2}, 108u, 54u);
    std::string dropout3 = g.AddNode<Dropout<ArrayType>>("dropout3", {fc3}, dropout_prob);
    // todo: load weights into the graph


//    g.AddNode<Relu<ArrayType>>("Relu1", {"FC1"});
//    g.AddNode<FullyConnected<ArrayType>>("FC2", {"Relu1"}, 10u, 10u);
//    g.AddNode<Relu<ArrayType>>("Relu2", {"FC2"});
//    g.AddNode<FullyConnected<ArrayType>>("FC3", {"Relu2"}, 10u, 10u);
//    g.AddNode<Softmax<ArrayType>>("Softmax", {"FC3"});

}