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

#include "ml/dataloaders/word2vec_loaders/w2v_dataloader.hpp"

#include <chrono>

using namespace std::chrono;

namespace fetch {
namespace ml {

template <typename ArrayType>
class W2VModel
{
private:
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using ContainerType = typename ArrayType::ContainerType;

  SizeType embeddings_size_;
  SizeType negative_;

  DataType alpha_;
  DataType starting_alpha_;

  ArrayType word_vector_;

  ArrayType                          embeddings_;
  ArrayType                          gradient_embeddings_;
  std::vector<fetch::math::SizeType> updated_rows_embeddings_;

  ArrayType                          weights_;
  ArrayType                          gradient_weights_;
  std::vector<fetch::math::SizeType> updated_rows_weights_;

  ArrayType target_weights_;
  ArrayType error_signal_;

  ArrayType error_words_;
  ArrayType error_target_weights_;

  W2VLoader<DataType> &data_loader_;

public:
  W2VModel(SizeType embeddings_size, SizeType negative, DataType starting_alpha,
           W2VLoader<DataType> &data_loader);

  void PrintStats(SizeType const &i, SizeType const &iter, SizeType const &iterations,
                  SizeType const &print_frequency, high_resolution_clock::time_point &last_time);

  void Train(SizeType iter, SizeType print_frequency, bool cbow = 1);

  ArrayType Embeddings();
};

template <typename ArrayType>
W2VModel<ArrayType>::W2VModel(SizeType embeddings_size, SizeType negative, DataType starting_alpha,
                              W2VLoader<DataType> &data_loader)
  : embeddings_size_(embeddings_size)
  , negative_(negative)
  , alpha_(starting_alpha)
  , starting_alpha_(starting_alpha)
  , data_loader_(data_loader)
{

  embeddings_          = ArrayType({embeddings_size_, data_loader.vocab_size()});
  gradient_embeddings_ = ArrayType({embeddings_size_, data_loader.vocab_size()});

  weights_          = ArrayType({embeddings_size_, data_loader.vocab_size()});
  gradient_weights_ = ArrayType({embeddings_size_, data_loader.vocab_size()});

  target_weights_ = ArrayType({embeddings_size_, 25});

  error_target_weights_ = ArrayType(target_weights_.shape());

  {  // Embeddings: Initialise
    fetch::random::LinearCongruentialGenerator rng;
    rng.Seed(42);
    for (auto &e : embeddings_)
    {
      e = DataType((rng.AsDouble() - 0.5) / static_cast<double>(embeddings_size));
    }
  }
  {  // Weights: Initialise
    fetch::random::LinearCongruentialGenerator rng;
    rng.Seed(42);
    fetch::math::TensorSliceIterator<DataType, ContainerType> it(weights_);
    while (it.is_valid())
    {
      *it = DataType(rng.AsDouble());
      ++it;
    }
  }
}

/**
 * Print training statistics
 * @tparam ArrayType
 * @param i
 * @param iter
 * @param iterations
 * @param print_frequency
 * @param last_time
 */
template <typename ArrayType>
void W2VModel<ArrayType>::PrintStats(SizeType const &i, SizeType const &iter,
                                     SizeType const &iterations, SizeType const &print_frequency,
                                     high_resolution_clock::time_point &last_time)
{
  high_resolution_clock::time_point cur_time = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(cur_time - last_time);
  std::cout << i << " / " << iter * iterations << " ("
            << static_cast<SizeType>(100.0 * static_cast<double>(i) /
                                     static_cast<double>(iter * iterations))
            << "%) -- "
            << "learning rate: " << alpha_ << " -- "
            << static_cast<double>(print_frequency) / time_span.count() << " words / sec"
            << std::endl;

  last_time = cur_time;
}

/**
 *
 * @param iter
 * @param print_frequency
 * @param cbow
 */
template <typename ArrayType>
void W2VModel<ArrayType>::Train(SizeType iter, SizeType print_frequency, bool cbow)
{
  if (cbow)
  {
    word_vector_ = ArrayType({embeddings_size_, 1});
  }
  else
  {
    word_vector_ = ArrayType({embeddings_size_, 2 * data_loader_.window_size()});
  }
  error_words_  = ArrayType(word_vector_.shape());
  error_signal_ = ArrayType({SizeType(negative_), 2 * data_loader_.window_size()});

  high_resolution_clock::time_point last_time;

  data_loader_.Reset();
  fetch::math::ApproxExpImplementation<0> fexp;

  data_loader_.GetNext();
  SizeType iterations = data_loader_.Size();

  // training loop
  for (SizeType i = 0; i < iter * iterations; ++i)
  {

    if (i % print_frequency == 0)
    {
      alpha_ =
          starting_alpha_ * ((static_cast<DataType>(iter * iterations) - static_cast<DataType>(i)) /
                             static_cast<DataType>(iter * iterations));
      if (alpha_ < starting_alpha_ * 0.0001)
      {
        alpha_ = static_cast<float>(starting_alpha_ * 0.0001);
      }

      PrintStats(i, iter, iterations, print_frequency, last_time);
    }

    if (data_loader_.IsDone())
    {
      data_loader_.Reset();
    }

    // Getting context and target
    auto  sample  = data_loader_.GetNext();
    auto &context = sample.first;
    auto &target  = sample.second;

    ///////////////////////
    /// FORWARD
    ///////////////////////

    if (cbow)
    {
      // Average Embeddings: context -> words
      SizeType valid_samples(0);
      auto     output_view = word_vector_.View(0);
      bool     clear       = true;
      for (DataType const &i : context)
      {
        if (i >= 0)
        {
          if (clear)
          {
            Assign(output_view, embeddings_.View(fetch::math::SizeType(i)));
            clear = false;
          }
          else
          {
            auto view = embeddings_.View(fetch::math::SizeType(i));
            PolyfillInlineAdd(output_view, view);
          }
          valid_samples++;
        }
      }

      DataType div = static_cast<DataType>(valid_samples);
      for (auto &val : output_view)
      {
        val /= div;
      }

      // Embeddings: target -> weights
      SizeType j = 0;

      for (DataType const &i : target)
      {
        auto view1 = target_weights_.View(j);
        auto view2 = weights_.View(fetch::math::SizeType(i));

        Assign(view1, view2);
        j++;
      }

      // MatrixMultiply: Forward
      fetch::math::TransposeDot(target_weights_, word_vector_, error_signal_);
    }

    else  // skip gram - one word vector per word - no averaging
    {
      // assign context embeddings
      // Embeddings: context -> words
      word_vector_.Reshape(
          {embeddings_size_, context.size()});  // reshape to maximum possible context size
      SizeType word_vector_idx = 0;
      SizeType valid_samples(0);
      for (DataType const &i : context)
      {
        auto output_view = word_vector_.View(word_vector_idx);
        if (i >= 0)
        {
          Assign(output_view, embeddings_.View(fetch::math::SizeType(i)));
          valid_samples++;
        }
        word_vector_idx++;
      }
      word_vector_.Reshape(
          {embeddings_size_, valid_samples});  // reshape back to only valid samples
      error_words_.Reshape(word_vector_.shape());
      error_signal_.Reshape({SizeType(negative_), valid_samples});

      // assign target weights (Embeddings: target -> weights)
      SizeType j = 0;

      for (DataType const &i : target)
      {
        auto view1 = target_weights_.View(j);
        auto view2 = weights_.View(fetch::math::SizeType(i));

        Assign(view1, view2);
        j++;
      }

      // MatrixMultiply: Forward
      fetch::math::TransposeDot(target_weights_, word_vector_, error_signal_);
    }

    ///////////////////////
    // ERROR
    ///////////////////////
    if (cbow)
    {
      for (SizeType cur_neg_sample = 0; cur_neg_sample < negative_; cur_neg_sample++)
      {
        DataType f     = error_signal_(cur_neg_sample, 0);
        DataType label = (cur_neg_sample == 0) ? 1 : 0;
        DataType sm    = static_cast<DataType>(static_cast<DataType>(fexp(f) / (1. + fexp(f))));
        error_signal_.Set(cur_neg_sample, 0, label - sm);
      }
    }
    else
    {
      for (SizeType cur_neg_sample = 0; cur_neg_sample < negative_; cur_neg_sample++)
      {
        for (SizeType cur_example = 0; cur_example < error_signal_.shape()[1]; cur_example++)
        {
          DataType f     = error_signal_(cur_neg_sample, cur_example);
          DataType label = (cur_neg_sample == 0) ? 1 : 0;
          DataType sm    = static_cast<DataType>(static_cast<DataType>(fexp(f) / (1. + fexp(f))));
          error_signal_.Set(cur_neg_sample, cur_example, label - sm);
        }
      }
    }

    ///////////////////////
    // BACKWARD
    ///////////////////////

    // MatrixMultiply: Backward
    fetch::math::Dot(target_weights_, error_signal_, error_words_);

    // TODO: for cbow the shape is diffeernt because of weight averaging
    fetch::math::DotTranspose(word_vector_, error_signal_, error_target_weights_);

    // Embeddings: Backward
    SizeType j = 0;
    for (DataType const &i : target)
    {
      auto ii = fetch::math::SizeType(double(i));
      updated_rows_weights_.push_back(ii);

      auto view1 = gradient_weights_.View(fetch::math::SizeType(double(i)));
      auto view2 = error_target_weights_.View(j);

      PolyfillInlineAdd(view1, view2);

      j++;
    }

    ///////////////////////
    // STEP
    ///////////////////////

    // Embeddings: Step in
    float learning_rate      = alpha_;
    using VectorRegisterType = typename fetch::math::TensorView<DataType>::VectorRegisterType;
    fetch::memory::TrivialRange range(0, std::size_t(gradient_weights_.height()));
    VectorRegisterType          rate(learning_rate);
    VectorRegisterType          zero(static_cast<DataType>(0));

    for (auto const &r : updated_rows_weights_)
    {
      auto input = gradient_weights_.View(r);
      auto ret   = weights_.View(r);  // embeddings.View(r);

      ret.data().in_parallel().Apply(
          range,
          [rate](VectorRegisterType const &a, VectorRegisterType const &b, VectorRegisterType &c) {
            c = b + a * rate;
          },
          input.data(), ret.data());
      input.data().in_parallel().Apply([zero](VectorRegisterType &a) { a = zero; });
    }

    updated_rows_weights_.clear();

    // Embeddings: Step
    for (auto const &r : updated_rows_embeddings_)
    {
      auto it1 = (gradient_embeddings_.View(r)).begin();
      auto it2 = (embeddings_.View(r)).begin();
      while (it1.is_valid())
      {
        *it2 += (*it1 * learning_rate);
        *it1 = 0;
        ++it1;
        ++it2;
      }
    }

    updated_rows_embeddings_.clear();
  }

  std::cout << "Done Training" << std::endl;
}

/**
 * Copies out embeddings
 * @tparam ArrayType
 * @return
 */
template <typename ArrayType>
ArrayType W2VModel<ArrayType>::Embeddings()
{
  return embeddings_;
}

}  // namespace ml
}  // namespace fetch