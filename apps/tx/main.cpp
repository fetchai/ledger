//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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


#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"

#include "core/json/document.hpp"
#include "core/script/variant.hpp"

#include "ledger/chain/wire_transaction.hpp"
#include "ledger/chain/helper_functions.hpp"
#include "core/byte_array/decoders.hpp"

#include <array>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct CommandLineArguments
{
  std::string input_json_tx_filename;

  static CommandLineArguments Parse(int argc, char **argv)
  {
    CommandLineArguments args;

    // define the parameters

    fetch::commandline::Params parameters;
    parameters.add(args.input_json_tx_filename, "f", "file name for json input TX data.", std::string{});

    // parse the args
    parameters.Parse(argc, argv);

    return args;
  }

  friend std::ostream &operator<<(std::ostream &              s,
                                  CommandLineArguments const &args)
  {
    s << '\n';
    s << "input tx file..: " << args.input_json_tx_filename << '\n';
    return s;
  }
};

void printRandomTx(std::size_t   num_of_resources  = 3,
                   int64_t const num_of_signatures = -4,
                   bool const    update_digest     = false)
{
  auto tx = fetch::chain::RandomTransaction(3,3);
  std::cout << tx << std::endl;
}

void verifyTx(fetch::serializers::ByteArrayBuffer &tx_data_stream)
{
  fetch::chain::MutableTransaction tx;
  auto txdata = fetch::chain::TxSigningAdapterFactory(tx);
  tx_data_stream >> txdata;


  if (tx.Verify(txdata))
  {
    std::cout << "SUCCESS: Transaction has been verified.";
  }
  else
  {
    std::cerr << "FAILURE: Verification of the transaction failed!" << std::endl;
    std::cerr << tx << std::endl;
    throw std::runtime_error("FAILURE: Verification of the transaction failed!");
  }
}

fetch::chain::MutableTransaction constructTxFromMetadata(fetch::script::Variant const &metadata_v)
{
  auto data = fetch::byte_array::FromBase64(metadata_v["data"].As<fetch::byte_array::ByteArray>());
  auto fee = metadata_v["fee"].As<double>();
  auto contract_name = metadata_v["contract_name"].As<fetch::byte_array::ByteArray>();

  auto resources_v = metadata_v["resources"];
  //if (resources_v.is_object)
  fetch::serializers::ByteArrayBuffer resources_stream{
    fetch::byte_array::FromBase64(resources_v.As<fetch::byte_array::ByteArray>())};
  std::set<fetch::byte_array::ConstByteArray> resources;
  resources_stream >> resources;

  fetch::serializers::ByteArrayBuffer private_keys_stream{
    fetch::byte_array::FromBase64(metadata_v["private_keys"].As<fetch::byte_array::ByteArray>())};
  std::set<fetch::byte_array::ConstByteArray> private_keys;
  resources_stream >> resources;

  //metadata_v["resources"];

  (void)fee;
  fetch::chain::MutableTransaction mtx;
  return mtx;
}


void handleProvidedTx(fetch::byte_array::ByteArray const &tx_jsom_string)
{
  fetch::json::JSONDocument tx_json{tx_jsom_string};
  auto &             tx_v = tx_json.root();

  auto data_v = tx_v["data"];
  if (data_v.is_string())
  {
    fetch::serializers::ByteArrayBuffer stream{
        fetch::byte_array::FromBase64(data_v.As<fetch::byte_array::ByteArray>())};
    verifyTx(stream);
  }
  else 
  {
    auto metadata_v = tx_v["metadata"];
    if (metadata_v.is_object())
    {
      auto mtx = constructTxFromMetadata(metadata_v);

    }
    else
    {
      std::cerr << "FAILURE: Input json structure does not contain neither `data` nor `metadata` attributes." << std::endl;
      throw std::runtime_error("FAILURE: Input json structure does not contain neither `data` nor `metadata` attributes.");
    }
  }
}

}  // namespace

int main(int argc, char **argv)
{
  try
  {
    auto const   args = CommandLineArguments::Parse(argc, argv);
    std::cout << args << std::endl;

    if (args.input_json_tx_filename.empty())
    {
      printRandomTx(3,3);
      return EXIT_SUCCESS;
    }

    std::string tx_json;
    if (args.input_json_tx_filename.rfind("{", 0) == 0)
    {
      tx_json = args.input_json_tx_filename;
    }
    else
    {
      std::ifstream istrm(args.input_json_tx_filename, std::ios::in);
      if (!istrm.is_open())
      {
        throw std::runtime_error("File \"" + args.input_json_tx_filename + "\" can not be oppened.");
      }
      std::stringstream buffer;
      buffer << istrm.rdbuf();
      tx_json = buffer.str();
    }
    handleProvidedTx(tx_json);

    return EXIT_SUCCESS;
  }
  catch (std::exception &ex)
  {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return EXIT_FAILURE;
}
