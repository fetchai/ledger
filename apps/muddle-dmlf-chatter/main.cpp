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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/json/document.hpp"
#include "core/serializers/main_serializer.hpp"
#include "dmlf/filepassing_learner_networker.hpp"
#include "dmlf/local_learner_networker.hpp"
#include "dmlf/muddle2_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "dmlf/update.hpp"
#include "dmlf/update_interface.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "variant/variant.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using DataType             = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType           = fetch::math::Tensor<DataType>;
using UpdateTypeForTesting = fetch::dmlf::Update<TensorType>;

class Learner
{
public:
  std::shared_ptr<fetch::dmlf::Muddle2LearnerNetworker>  actual;
  std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> interface;

  Learner(const std::string &cloud_config, std::size_t instance_number)
  {
    actual = std::make_shared<fetch::dmlf::Muddle2LearnerNetworker>(cloud_config, instance_number);
    interface = actual;
    interface->Initialize<UpdateTypeForTesting>();
  }

  void PretendToLearn()
  {
    static int sequence_number = 1;
    auto       t               = TensorType(TensorType::SizeType(2));
    t.Fill(DataType(sequence_number++));
    auto r = std::vector<TensorType>();
    r.push_back(t);
    interface->PushUpdate(std::make_shared<UpdateTypeForTesting>(r));
  }
};
using Muddle2LearnerNetworker = fetch::dmlf::Muddle2LearnerNetworker;

int main(int /*argc*/, char **argv)
{
  auto config          = std::string(argv[1]);
  auto instance_number = std::atoi(argv[2]);

  std::cout << "CONFIG:" << config << std::endl;

  auto learner = std::make_shared<Learner>(config, instance_number);

  sleep(1);
  if (instance_number == 0)
  {
    learner->PretendToLearn();
  }

  sleep(1);

  int r;
  if (std::size_t(instance_number) == learner->actual->GetUpdateCount())
  {
    std::cout << "yes" << std::endl;
    r = 0;
  }
  else
  {
    std::cout << "no" << std::endl;
    r = 1;
  }

  return r;
}
