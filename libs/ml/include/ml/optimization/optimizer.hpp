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

#include "ml/graph.hpp"

namespace fetch {
    namespace ml {

    template <class T>
    class Optimizer{
    public:
        using ArrayType          = T;
        using DataType = typename ArrayType::Type;
        using SizeType = typename ArrayType::SizeType;

    Optimizer(std::shared_ptr<Graph<T>> error,std::string const &output_node_name,DataType const &learning_rate)
    : error_(error)
    , output_node_name_(output_node_name)
    , learning_rate_(learning_rate)
    {
        auto weights=error_->GetWeights();
        for(auto &wei : weights)
        {
        momentum_.push_back(ArrayType(wei.shape()));
        }

    }

        ArrayType Step()
        {
           ArrayType error_signal = error_->Evaluate(output_node_name_);
           error_->BackPropagate(output_node_name_, error_signal);


           /*
            std::vector<ArrayType> gradients = error_->GetGradients();

            // Do operation with gradient
            SizeType i{0};
            for (auto &grad : gradients)
            {
                fetch::math::Add(grad, momentum_[i], grad);
                fetch::math::Multiply(grad, -learning_rate_, grad);
                i++;
            }
            error_->ApplyGradients(gradients);
            */
           error_->Step(learning_rate_);

            return error_signal;
        }

    private:
        std::shared_ptr<Graph<T>> error_;
        std::string output_node_name_;
        DataType learning_rate_;
        std::vector<ArrayType> momentum_;

        void ResetMomentum()
        {
            for(auto &moment : momentum_)
            {
                moment.Fill(DataType{0});
            }
        }

    };


    }
    }