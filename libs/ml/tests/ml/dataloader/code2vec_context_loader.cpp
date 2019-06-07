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

#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/code2vec_context_loaders/context_loader.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>
#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

TEST(C2vLoaderTest, loader_test)
{
  using ArrayType   = typename fetch::math::Tensor<fetch::math::SizeType>;
  using SizeType    = fetch::math::SizeType;
  using LabelType   = ArrayType;
  using ContextType = ArrayType;

  std::string training_data =
      "get|timestamp override,-726273290,long override,-733851942,METHOD_NAME "
      "long,1625949899,METHOD_NAME long,-54675710,timestamp METHOD_NAME,263491700,timestamp\n"
      "get|type override,-1057165453,type override,-733851942,METHOD_NAME "
      "type,1387642418,METHOD_NAME type,774787451,type METHOD_NAME,263491700,type\n"
      "to|string override,-1057165453,string override,-733851942,METHOD_NAME "
      "string,1387642418,METHOD_NAME string,-1268584042,eventtype string,1373885347,gettype "
      "METHOD_NAME,-365551875,eventtype METHOD_NAME,736086556,gettype "
      "eventtype,1878569167,gettype\n"
      "pre|head override,-678980855,void override,-733851942,METHOD_NAME "
      "void,-1543468728,METHOD_NAME void,-201269925,html void,1680444497,page void,1680444528,_ "
      "METHOD_NAME,1261040172,html METHOD_NAME,-347104160,page METHOD_NAME,-347104129,_ "
      "METHOD_NAME,-1682731935,html METHOD_NAME,-1682731904,commonprehead html,-1499537329,page "
      "html,-1499537298,_ html,1990060077,html html,1990060108,commonprehead page,893479,_ "
      "page,-510609304,html page,-510609273,commonprehead _,1470552775,html "
      "_,1470552806,commonprehead html,-1923383765,commonprehead\n"
      "content override,-1256194184,subview override,-733851942,METHOD_NAME "
      "subview,1466431311,METHOD_NAME subview,-1710811525,aboutblock "
      "METHOD_NAME,-345275919,aboutblock";

  SizeType max_contexts = 10;

  C2VLoader<LabelType, ContextType> loader(max_contexts);

  loader.AddData(training_data);

  EXPECT_EQ(loader.function_name_counter().size(), 5);
  EXPECT_EQ(loader.path_counter().size(), 37);
  EXPECT_EQ(loader.word_counter().size(), 15);

  EXPECT_EQ(loader.umap_idx_to_functionname()[0], "EMPTY_CONTEXT_STRING");
  EXPECT_EQ(loader.umap_idx_to_functionname()[1], "get|timestamp");
  EXPECT_EQ(loader.umap_idx_to_functionname()[2], "get|type");
  EXPECT_EQ(loader.umap_idx_to_functionname()[3], "to|string");
  EXPECT_EQ(loader.umap_idx_to_functionname()[4], "pre|head");
  EXPECT_EQ(loader.umap_idx_to_functionname()[5], "content");
  EXPECT_EQ(loader.umap_idx_to_functionname()[6], "");
  EXPECT_EQ(loader.umap_idx_to_functionname()[7], "");

  typename C2VLoader<LabelType, ContextType>::ContextTensorsLabelPair training_pair =
      loader.GetNext();

  EXPECT_EQ(training_pair.second.at(0).size(), max_contexts);
  EXPECT_EQ(training_pair.second.at(1).size(), max_contexts);
  EXPECT_EQ(training_pair.second.at(2).size(), max_contexts);

  EXPECT_EQ(training_pair.second.at(0).At(0, 0), 1);
  EXPECT_EQ(training_pair.second.at(0).At(1, 0), 1);
  EXPECT_EQ(training_pair.second.at(0).At(2, 0), 2);
  EXPECT_EQ(training_pair.second.at(0).At(3, 0), 2);
  EXPECT_EQ(training_pair.second.at(0).At(4, 0), 3);
  EXPECT_EQ(training_pair.second.at(0).At(5, 0), 0);
  EXPECT_EQ(training_pair.second.at(0).At(6, 0), 0);
  EXPECT_EQ(training_pair.second.at(0).At(7, 0), 0);
  EXPECT_EQ(training_pair.second.at(0).At(8, 0), 0);
  EXPECT_EQ(training_pair.second.at(0).At(9, 0), 0);

  EXPECT_EQ(training_pair.second.at(1).At(0, 0), 1);
  EXPECT_EQ(training_pair.second.at(1).At(1, 0), 2);
  EXPECT_EQ(training_pair.second.at(1).At(2, 0), 3);
  EXPECT_EQ(training_pair.second.at(1).At(3, 0), 4);
  EXPECT_EQ(training_pair.second.at(1).At(4, 0), 5);
  EXPECT_EQ(training_pair.second.at(1).At(5, 0), 0);
  EXPECT_EQ(training_pair.second.at(1).At(6, 0), 0);
  EXPECT_EQ(training_pair.second.at(1).At(7, 0), 0);
  EXPECT_EQ(training_pair.second.at(1).At(8, 0), 0);
  EXPECT_EQ(training_pair.second.at(1).At(9, 0), 0);

  EXPECT_EQ(training_pair.second.at(2).At(0, 0), 2);
  EXPECT_EQ(training_pair.second.at(2).At(1, 0), 3);
  EXPECT_EQ(training_pair.second.at(2).At(2, 0), 3);
  EXPECT_EQ(training_pair.second.at(2).At(3, 0), 4);
  EXPECT_EQ(training_pair.second.at(2).At(4, 0), 4);
  EXPECT_EQ(training_pair.second.at(2).At(5, 0), 0);
  EXPECT_EQ(training_pair.second.at(2).At(6, 0), 0);
  EXPECT_EQ(training_pair.second.at(2).At(7, 0), 0);
  EXPECT_EQ(training_pair.second.at(2).At(8, 0), 0);
  EXPECT_EQ(training_pair.second.at(2).At(9, 0), 0);
}
