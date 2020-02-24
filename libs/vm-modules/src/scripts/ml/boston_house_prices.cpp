//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "vm_modules/scripts/ml/boston_house_prices.hpp"

namespace vm_modules {
namespace scripts {
namespace ml {

std::string BostonHousingScript(std::string &batch_size, bool load_data)
{

  std::string script = R"(
    function main()
    )";

  std::string data_script;
  if (load_data)
  {
    data_script = R"(
        // read in training and test data
        if (System.Argc() != 5)
          print("Usage: SCRIPT_FILE -- PATH/TO/BOSTON_TRAIN_DATA.CSV PATH/TO/BOSTON_TRAIN_LABELS.CSV PATH/TO/BOSTON_TEST_DATA.CSV PATH/TO/BOSTON_TEST_LABELS.CSV ");
          return;
        endif
        var data = readCSV(System.Argv(1));
        var label = readCSV(System.Argv(2));
        var test_data = readCSV(System.Argv(3));
        var test_label = readCSV(System.Argv(4));
      )";
  }
  else
  {
    data_script = R"(
        // read in training and test data
        var data = Tensor('0.00632,18.0;2.31,0.0;0.538,6.575;65.2,4.09;1.0,296.0;15.3,396.9;4.98,0.00632;18.0,2.31;0.0,0.538;6.575,65.2;4.09,1.0;296.0,15.3;396.9,4.98;');
        var label = Tensor('24.0,24.0;');
      )";
  }

  std::string model_script = R"(

      // set up a model architecture
      var model = Model("sequential");
      model.add("dense", 13u64, 10u64, "relu");
      model.add("dense", 10u64, 10u64, "relu");
      model.add("dense", 10u64, 1u64);
      model.compile("mse", "adam");
      )";

  std::string batch_script = R"(
      var batch_size =)";
  std::string end_line     = R"(;)";

  std::string work_script = R"(
      // train the model
      model.fit(data, label, batch_size);

      // evaluate performance
      var loss = model.evaluate();

      // make predictions on data
      var predictions = model.predict(data);
      print(predictions.at(0u64, 0u64));

    endfunction
  )";

  std::ostringstream ss;
  ss << script << data_script << model_script << batch_script << batch_size << end_line
     << work_script;
  std::string result = ss.str();

  return result;
}

}  // namespace ml
}  // namespace scripts
}  // namespace vm_modules
