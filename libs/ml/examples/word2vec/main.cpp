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

#include "math/distance/cosine.hpp"
#include "ml/dataloaders/w2v_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy.hpp"

#include <iostream>
#include <math/tensor.hpp>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using DataType     = double;
using ArrayType    = fetch::math::Tensor<DataType>;
using ArrayPtrType = std::shared_ptr<ArrayType>;
using SizeType     = typename ArrayType::SizeType;

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct PARAMS
{
  SizeType batch_size     = 128;      // training data batch size
  SizeType embedding_size = 10;       // dimension of embedding vec
  SizeType training_steps = 1280000;  // total number of training steps
  double   learning_rate  = 0.01;     // alpha - the learning rate
  bool     cbow           = false;    // skipgram model if false, cbow if true
  SizeType skip_window    = 5;        // max size of context window one way
  SizeType super_samp     = 10;       // n times to reuse an input to generate a label
  SizeType k_neg_samps    = 15;       // number of negative examples to sample
  double   discard_thresh = 0.003;    // controls how aggressively to discard frequent words
};

std::string TRAINING_DATA =
    "The Ugly Duckling.\n"
    "\n"
    "A duck made her nest under some leaves.\n"
    "She sat on the eggs to keep them warm.\n"
    "At last the eggs broke, one after the other. Little ducks came out.\n"
    "Only one egg was left. It was a very large one.\n"
    "At last it broke, and out came a big, ugly duckling.\n"
    "\"What a big duckling!\" said the old duck. \"He does not look like us. Can he be a turkey? "
    "We will see. If he does not like the water, he is not a duck.\"\n"
    "\n"
    "The next day the mother duck took her ducklings to the pond.\n"
    "Splash! Splash! The mother duck was in the water. Then she called the ducklings to come in. "
    "They all jumped in and began to swim. The big, ugly duckling swam, too.\n"
    "The mother duck said, \"He is not a turkey. He is my own little duck. He will not be so ugly "
    "when he is bigger.\"\n"
    "\n"
    "Then she said to the ducklings, \"Come with me. I want you to see the other ducks. Stay by me "
    "and look out for the cat.\"\n"
    "They all went into the duck yard. What a noise the ducks made!\n"
    "While the mother duck was eating a big bug, an old duck bit the ugly duckling.\n"
    "\"Let him alone,\" said the mother duck. \"He did not hurt you.\"\n"
    "\"I know that,\" said the duck, \"but he is so ugly, I bit him.\"\n"
    "\n"
    "The next duck they met, said, \"You have lovely ducklings. They are all pretty but one. He is "
    "very ugly.\"\n"
    "The mother duck said, \"I know he is not pretty. But he is very good.\"\n"
    "Then she said to the ducklings, \"Now, my dears, have a good time.\"\n"
    "But the poor, big, ugly duckling did not have a good time.\n"
    "The hens all bit him. The big ducks walked on him.\n"
    "The poor duckling was very sad. He did not want to be so ugly. But he could not help it.\n"
    "He ran to hide under some bushes. The little birds in the bushes were afraid and flew away.\n"
    "\n"
    "\"It is all because I am so ugly,\" said the duckling. So he ran away.\n"
    "At night he came to an old house. The house looked as if it would fall down. It was so old. "
    "But the wind blew so hard that the duckling went into the house.\n"
    "An old woman lived there with her cat and her hen.\n"
    "The old woman said, \"I will keep the duck. I will have some eggs.\"\n"
    "\n"
    "The next day, the cat saw the duckling and began to growl.\n"
    "The hen said, \"Can you lay eggs?\" The duckling said, \"No.\"\n"
    "\"Then keep still,\" said the hen. The cat said, \"Can you growl?\"\n"
    "\"No,\" said the duckling.\n"
    "\"Then keep still,\" said the cat.\n"
    "And the duckling hid in a corner. The next day he went for a walk. He saw a big pond. He "
    "said, \"I will have a good swim.\"\n"
    "But all of the animals made fun of him. He was so ugly.\n"
    "\n"
    "The summer went by.\n"
    "Then the leaves fell and it was very cold. The poor duckling had a hard time.\n"
    "It is too sad to tell what he did all winter.\n"
    "At last it was spring.\n"
    "The birds sang. The ugly duckling was big now.\n"
    "One day he flew far away.\n"
    "Soon he saw three white swans on the lake.\n"
    "He said, \"I am going to see those birds. I am afraid they will kill me, for I am so ugly.\"\n"
    "He put his head down to the water. What did he see? He saw himself in the water. But he was "
    "not an ugly duck. He was a white swan.\n"
    "The other swans came to see him.\n"
    "The children said, \"Oh, see the lovely swans. The one that came last is the best.\"\n"
    "And they gave him bread and cake.\n"
    "It was a happy time for the ugly duckling.";

////////////////////////
/// MODEL DEFINITION ///
////////////////////////

std::string Model(fetch::ml::Graph<ArrayType> &g, SizeType vocab_size, SizeType embeddings_size)
{
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Context", {});
  std::string ret_name = g.AddNode<fetch::ml::layers::SkipGram<ArrayType>>(
      "SkipGram", {"Input", "Context"}, vocab_size, SizeType(1), embeddings_size);

  return ret_name;
}

std::vector<DataType> TestEmbeddings(fetch::ml::Graph<ArrayType> &g, std::string &skip_gram_name,
                                     fetch::ml::W2VLoader<ArrayType> &dl)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  // cat
  ArrayPtrType embed_cat_input = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  cat_lookup      = "cat";
  SizeType     cat_idx         = dl.VocabLookup(cat_lookup);
  embed_cat_input->At(cat_idx) = 1;
  ArrayType cat_output         = embeddings->Forward({embed_cat_input})->Clone();

  ArrayPtrType embed_duckling_input      = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  duckling_lookup           = "duckling";
  SizeType     duckling_idx              = dl.VocabLookup(duckling_lookup);
  embed_duckling_input->At(duckling_idx) = 1;
  ArrayType duckling_output              = embeddings->Forward({embed_duckling_input})->Clone();

  ArrayPtrType embed_ugly_input  = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  ugly_lookup       = "ugly";
  SizeType     ugly_idx          = dl.VocabLookup(ugly_lookup);
  embed_ugly_input->At(ugly_idx) = 1;
  ArrayType ugly_output          = embeddings->Forward({embed_ugly_input})->Clone();

  ArrayPtrType embed_egg_input = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  egg_lookup      = "egg";
  SizeType     egg_idx         = dl.VocabLookup(egg_lookup);
  embed_egg_input->At(egg_idx) = 1;
  ArrayType egg_output         = embeddings->Forward({embed_egg_input})->Clone();

  DataType result_cat_duckling, result_cat_ugly, result_duckling_ugly, result_duckling_egg;

  // distance from cat to duckling (using MSE as distance)
  result_cat_duckling = fetch::math::distance::Cosine(cat_output, duckling_output);

  // distance from cat to ugly (using MSE as distance)
  result_cat_ugly = fetch::math::distance::Cosine(cat_output, ugly_output);

  // distance from duckling to ugly (using MSE as distance)
  result_duckling_ugly = fetch::math::distance::Cosine(duckling_output, ugly_output);

  // distance from duckling to egg (using MSE as distance)
  result_duckling_egg = fetch::math::distance::Cosine(duckling_output, egg_output);

  std::vector<DataType> ret = {result_cat_duckling, result_cat_ugly, result_duckling_ugly,
                               result_duckling_egg};

  std::cout << "cat-duckling distance: " << ret[0] << std::endl;
  std::cout << "cat-ugly distance: " << ret[1] << std::endl;
  std::cout << "duckling-ugly distance: " << ret[2] << std::endl;
  std::cout << "duckling-egg distance: " << ret[3] << std::endl;

  return ret;
}

int main()
{

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  PARAMS p;

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  // set up dataloader
  std::cout << "Setting up training data...: " << std::endl;
  fetch::ml::W2VLoader<ArrayType> dataloader(TRAINING_DATA, p.cbow, p.skip_window, p.super_samp,
                                             p.k_neg_samps, p.discard_thresh);

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  fetch::ml::Graph<ArrayType> g;
  std::string                 output_name = Model(g, dataloader.VocabSize(), p.embedding_size);

  // set up loss
  SoftmaxCrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  std::pair<std::pair<std::shared_ptr<ArrayType>, std::shared_ptr<ArrayType>>, SizeType> input;
  std::shared_ptr<ArrayType>                                                             gt =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 1}));
  DataType loss = 0;

  for (std::size_t i = 0; i < p.training_steps; ++i)
  {
    input = dataloader.GetRandom();

    g.SetInput("Input", input.first.first);
    g.SetInput("Context", input.first.second);
    gt->Fill(0);
    gt->At(0)                          = DataType(input.second);
    std::shared_ptr<ArrayType> results = g.Evaluate(output_name);

    std::shared_ptr<ArrayType> result =
        std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 1}));

    DataType tmp_loss = criterion.Forward({results, gt});
    if (input.second == 0)
    {
      tmp_loss *= DataType(p.k_neg_samps);
    }

    loss += tmp_loss;

    g.BackPropagate(output_name, criterion.Backward({results, gt}));

    if (i % p.batch_size == (p.batch_size - 1))
    {
      std::cout << "MiniBatch: " << i / p.batch_size << " -- Loss : " << loss << std::endl;
      g.Step(p.learning_rate);
      loss = 0;
    }

    if (i % (p.batch_size * 100) == ((p.batch_size * 100) - 1))
    {
      // Test trained embeddings
      std::vector<DataType> trained_distances = TestEmbeddings(g, output_name, dataloader);
    }
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  std::vector<DataType> trained_distances = TestEmbeddings(g, output_name, dataloader);

  std::cout << "final cat-duckling distance: " << trained_distances[0] << std::endl;
  std::cout << "final cat-ugly distance: " << trained_distances[1] << std::endl;
  std::cout << "final duckling-ugly distance: " << trained_distances[2] << std::endl;
  std::cout << "final duckling-egg distance: " << trained_distances[3] << std::endl;

  return 0;
}
