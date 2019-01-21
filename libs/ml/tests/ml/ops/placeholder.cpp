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

#include "ml/ops/placeholder.hpp"
#include "math/ndarray.hpp"
#include <gtest/gtest.h>

TEST(placeholder_test, setData)
{
  std::shared_ptr<fetch::math::NDArray<int>> data = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> gt   = std::make_shared<fetch::math::NDArray<int>>(8);
  size_t                                     i(0);
  for (int e : {1, 2, 3, 4, 5, 6, 7, 8})
  {
    data->Set(i, e);
    gt->Set(i, e);
    i++;
  }
  fetch::ml::ops::PlaceHolder<fetch::math::NDArray<int>> op;
  op.SetData(data);
  std::shared_ptr<fetch::math::NDArray<int>> prediction = op.Forward({});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(placeholder_test, resetData)
{
  std::shared_ptr<fetch::math::NDArray<int>> data = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> gt   = std::make_shared<fetch::math::NDArray<int>>(8);
  {
    size_t i(0);
    for (int e : {1, 2, 3, 4, 5, 6, 7, 8})
    {
      data->Set(i, e);
      gt->Set(i, e);
      i++;
    }
  }
  fetch::ml::ops::PlaceHolder<fetch::math::NDArray<int>> op;
  op.SetData(data);
  std::shared_ptr<fetch::math::NDArray<int>> prediction = op.Forward({});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));

  // reset
  {
    size_t i(0);
    for (int e : {12, 13, -14, 15, 16, -17, 18, 19})
    {
      data->Set(i, e);
      gt->Set(i, e);
      i++;
    }
  }

  op.SetData(data);
  prediction = op.Forward({});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}
