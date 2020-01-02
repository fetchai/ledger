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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/main_serializer.hpp"
#include "json/document.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/serializers/ml_types.hpp"
#include "variant/variant.hpp"
#include "vm/io_observer_interface.hpp"
#include "vm/module.hpp"
#include "vm/variant.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/system.hpp"
#include "vm_modules/math/read_csv.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/ml/ml.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using DataType   = fetch::vm_modules::math::VMTensor::DataType;
using TensorType = fetch::math::Tensor<DataType>;
using System     = fetch::vm_modules::System;

using fetch::byte_array::FromHex;
using fetch::byte_array::ToHex;

std::string ReadFileContents(std::string const &path)
{
  std::ifstream f(path.c_str());
  std::string   str{};

  if (!f.is_open())
  {
    return str;
  }

  // pre-allocate the string buffer
  f.seekg(0, std::ios::end);
  auto const size = static_cast<std::size_t>(f.tellg());
  if (size == 0)
  {
    return str;
  }

  str.reserve(size);
  f.seekg(0, std::ios::beg);

  // assign the contents
  str.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

  return str;
}

struct JsonStateMap : public fetch::vm::IoObserverInterface
{
public:
  // Construction / Destruction
  JsonStateMap()                     = default;
  JsonStateMap(JsonStateMap const &) = delete;
  JsonStateMap(JsonStateMap &&)      = delete;
  ~JsonStateMap() override           = default;

  /// @name Save and Restore Operations
  /// @{
  void LoadFromFile(char const *filename)
  {
    // read the contents of the file
    fetch::byte_array::ConstByteArray file_contents{ReadFileContents(filename)};

    if (!file_contents.empty())
    {
      // parse the contents of the file
      fetch::json::JSONDocument document{file_contents};

      if (!document.root().IsObject())
      {
        throw std::runtime_error("JSON state file is not correct");
      }

      // load the file
      data_ = document.root();
    }
  }

  void SaveToFile(char const *filename)
  {
    std::ofstream file{filename};
    file << data_;
  }
  /// @}

  /// @name Io Observable Interface
  /// @{
  Status Read(std::string const &key, void *data, uint64_t &size) override
  {
    Status status{Status::ERROR};

    if (data_.Has(key))
    {
      auto const value = FromHex(data_[key].As<fetch::byte_array::ConstByteArray>());

      status = Status::BUFFER_TOO_SMALL;
      if (size >= value.size())
      {
        auto *raw_data = reinterpret_cast<uint8_t *>(data);
        std::memcpy(raw_data, value.pointer(), value.size());

        status = Status::OK;
      }

      // update the output size
      size = value.size();
    }

    return status;
  }

  Status Write(std::string const &key, void const *data, uint64_t size) override
  {
    auto const *raw_data = reinterpret_cast<uint8_t const *>(data);

    // store the data key
    data_[key] = ToHex(fetch::byte_array::ConstByteArray(raw_data, size));

    return Status::OK;
  }

  Status Exists(std::string const &key) override
  {
    return (data_.Has(key) ? Status::OK : Status::ERROR);
  }
  /// @}

  fetch::variant::Variant const &data()
  {
    return data_;
  }

  // Operators
  JsonStateMap &operator=(JsonStateMap const &) = delete;
  JsonStateMap &operator=(JsonStateMap &&) = delete;

private:
  fetch::variant::Variant data_{fetch::variant::Variant::Object()};
};

int RunEtchScript(std::string const &filename, std::shared_ptr<fetch::vm::Module> const &module)
{
  std::cout << "Running etch script " << filename << std::endl;

  /// Setting compiler up
  auto                     compiler = std::make_unique<fetch::vm::Compiler>(module.get());
  fetch::vm::Executable    executable;
  fetch::vm::IR            ir;
  std::vector<std::string> errors;

  /// Reading etch file
  std::ifstream file(filename, std::ios::binary);
  if (file.fail())
  {
    std::cout << "Cannot open file " << filename << std::endl;
    return -1;
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  /// Compiling
  fetch::vm::SourceFiles files    = {{"default.etch", source}};
  bool                   compiled = compiler->Compile(files, "default_ir", ir, errors);

  if (!compiled)
  {
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  /// set up VM
  auto vm = std::make_shared<fetch::vm::VM>(module.get());
  vm->SetChargeLimit(fetch::vm::ChargeAmount(0));

  // attach observer so that writing to state works
  JsonStateMap observer{};
  try
  {
    observer.LoadFromFile("myfile.json");
  }
  catch (std::exception &e)
  {
    std::cout << "Can not load JSON file :" << e.what() << std::endl;
    return -1;
  }

  vm->SetIOObserver(observer);

  // attach std::cout for printing
  vm->AttachOutputDevice(fetch::vm::VM::STDOUT, std::cout);

  /// executing
  if (!vm->GenerateExecutable(ir, "default_exe", executable, errors))
  {
    std::cout << "Failed to generate executable" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (executable.FindFunction("main") == nullptr)
  {
    std::cout << "Function 'main' not found" << std::endl;
    return -2;
  }

  // Setting VM up and running
  std::string        error;
  fetch::vm::Variant output;

  if (!vm->Execute(executable, "main", error, output))
  {
    std::cout << "Runtime error on line " << error << std::endl;
    return -3;
  }

  observer.SaveToFile("myfile.json");

  return 0;
}

int main(int argc, char **argv)
{
  // parse the command line parameters
  System::Parse(argc, argv);

  fetch::commandline::ParamsParser const &pp = System::GetParamsParser();

  // ensure the program has the correct number of args
  if (3u != pp.arg_size())
  {
    std::cerr << "Usage: " << pp.GetArg(0)
              << " <etch_saver_filename> <etch_loader_filename> -- [script args...]" << std::endl;
    return 1;
  }

  std::string etch_saver  = pp.GetArg(1);
  std::string etch_loader = pp.GetArg(2);

  /// Set up module
  auto module = std::make_shared<fetch::vm::Module>();

  fetch::vm_modules::System::Bind(*module);

  fetch::vm_modules::ml::BindML(*module, true);

  fetch::vm_modules::CreatePrint(*module);

  fetch::vm_modules::math::BindReadCSV(*module, true);

  RunEtchScript(etch_saver, module);
  RunEtchScript(etch_loader, module);

  return 0;
}
