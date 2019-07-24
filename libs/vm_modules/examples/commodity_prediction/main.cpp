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
#include "core/json/document.hpp"
#include "core/serializers/byte_array.hpp"
#include "ledger/state_adapter.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "variant/variant.hpp"
#include "vm/io_observer_interface.hpp"
#include "vm/module.hpp"
#include "vm/variant.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/ml/ml.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using DataType  = typename fetch::vm_modules::math::VMTensor::DataType;
using ArrayType = fetch::math::Tensor<DataType>;

struct System : public fetch::vm::Object
{
  System()           = delete;
  ~System() override = default;

  static int32_t Argc(fetch::vm::VM * /*vm*/, fetch::vm::TypeId /*type_id*/)
  {
    return int32_t(System::args.size());
  }

  static fetch::vm::Ptr<fetch::vm::String> Argv(fetch::vm::VM *vm, fetch::vm::TypeId /*type_id*/,
                                                int32_t        a)
  {
    return fetch::vm::Ptr<fetch::vm::String>(
        new fetch::vm::String(vm, System::args[std::size_t(a)]));
  }

  static std::vector<std::string> args;
};

std::vector<std::string> System::args;

// read the weights and bias csv files

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> read_csv(
    fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &filename, bool transpose = false)
{
  ArrayType weights = fetch::ml::dataloaders::ReadCSV<ArrayType>(filename->str, 0, 0, transpose);
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(weights);
}

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
    throw std::runtime_error("Cannot open file " + std::string(filename));
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  /// Compiling
  bool compiled = compiler->Compile(source, "myexecutable", ir, errors);

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

  // attach observer so that writing to state works
  JsonStateMap observer{};
  observer.LoadFromFile("myfile.json");
  vm->SetIOObserver(observer);

  // attach std::cout for printing
  vm->AttachOutputDevice(fetch::vm::VM::STDOUT, std::cout);

  /// executing
  if (!vm->GenerateExecutable(ir, "main_ir", executable, errors))
  {
    std::cout << "Failed to generate executable" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!executable.FindFunction("main"))
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
  if (argc < 3)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    std::exit(-9);
  }

  for (int i = 3; i < argc; ++i)
  {
    System::args.emplace_back(std::string(argv[i]));
  }

  std::string etch_saver  = argv[1];
  std::string etch_loader = argv[2];

  /// Set up module
  auto module = std::make_shared<fetch::vm::Module>();

  module->CreateClassType<System>("System")
      .CreateStaticMemberFunction("Argc", &System::Argc)
      .CreateStaticMemberFunction("Argv", &System::Argv);

  fetch::vm_modules::ml::BindML(*module);

  fetch::vm_modules::CreatePrint(*module);

  module->CreateFreeFunction("read_csv", &read_csv);

  RunEtchScript(etch_saver, module);
  RunEtchScript(etch_loader, module);

  return 0;
}
