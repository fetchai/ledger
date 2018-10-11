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

using PrivateKeys = std::vector<fetch::byte_array::ConstByteArray>; 

template<typename STREAM>
void printSeparator(STREAM& stream, std::string const& desc = std::string{})
{
  static std::string const line_separ("================================================================================");
  stream << line_separ << std::endl;
  if (!desc.empty())
  {
    static std::string const prefix("====   ");
    stream << prefix << desc;

    constexpr std::size_t sfx_space_len = 3;
    std::size_t const left_side_len = prefix.size() + desc.size();
    if (left_side_len <= line_separ.size() - sfx_space_len)
    {
      stream << "   " << std::string((line_separ.size() - sfx_space_len - left_side_len), '=');
    }
    stream << std::endl;
  }
}

struct CommandLineArguments
{
  std::string input_json_tx_filename;
  std::string priv_keys_filename;
  bool is_verbose = false;

  static CommandLineArguments Parse(int argc, char **argv)
  {
    CommandLineArguments args;

    // define the parameters

    fetch::commandline::Params parameters;
    parameters.add(args.input_json_tx_filename, "f", "file name for json input TX data.", std::string{});
    parameters.add(args.priv_keys_filename, "p", "file name for prvate keys in json format. Two private kyes will be generated *IF* this option is *NOT* provided.", std::string{});
    parameters.add(args.is_verbose, "v", "enables verbose output printing out details", false);

    // parse the args
    parameters.Parse(argc, argv);

    return args;
  }

  friend std::ostream &operator<<(std::ostream &              s,
                                  CommandLineArguments const &args)
  {
    printSeparator(s, "COMANDLINE ARGUMENTS");
    s << "input tx file          : " << args.input_json_tx_filename << std::endl;
    s << "input private keys file: " << args.priv_keys_filename << std::endl;
    s << "verbose                : " << args.is_verbose << std::endl;
    return s;
  }
};

void printTx(fetch::chain::MutableTransaction const& tx, std::string const& desc = std::string{}, bool const &is_verbose=false)
{
  if (is_verbose)
  {
    printSeparator(std::cout, desc);
  }
  std::cout << fetch::chain::ToWireTransaction(tx, true) << std::endl;
}

void verifyTx(fetch::serializers::ByteArrayBuffer &tx_data_stream)
{
  fetch::chain::MutableTransaction tx;
  auto txdata = fetch::chain::TxSigningAdapterFactory(tx);
  tx_data_stream >> txdata;

  if (tx.Verify(txdata))
  {
    std::cout << "SUCCESS: Transaction has been verified." << std::endl;
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
  fetch::chain::MutableTransaction mtx;
  mtx.set_contract_name(metadata_v["contract_name"].As<fetch::byte_array::ByteArray>());
  mtx.set_data(fetch::byte_array::FromBase64(metadata_v["data"].As<fetch::byte_array::ByteArray>()));
  mtx.set_fee(metadata_v["fee"].As<uint64_t>());
  auto resources_v = metadata_v["resources"];
  auto private_keys_v = metadata_v["private_keys"];

  auto &resources = mtx.resources();
  if (resources_v.is_array())
  {
      resources_v.ForEach([&](fetch::script::Variant &value) -> bool {
        auto const result = resources.emplace(fetch::byte_array::FromBase64(value.As<fetch::byte_array::ByteArray>()));
        if (!result.second)
        {
          std::cerr << "WARNING: Atempt to insert already existing resource \"" << *result.first << "\"!" << std::endl;
        }
        return true;
    });
  }
  else if (!resources_v.is_undefined())
  {
    std::cerr << "WARNING: the `resources` attribute has been ignored due to its unexpected type." << std::endl;
  }
  
  std::set<fetch::byte_array::ConstByteArray> private_keys;
  if (private_keys_v.is_array())
  {
      private_keys_v.ForEach([&](fetch::script::Variant &value) -> bool {
        auto const result = private_keys.emplace(fetch::byte_array::FromBase64(value.As<fetch::byte_array::ByteArray>()));
        if (!result.second)
        {
          std::cerr << "WARNING: Atempt to insert already existing key \"" << *result.first << "\"!" << std::endl;
        }
        return true;
    });
  }
  else if (!private_keys_v.is_undefined())
  {
    std::cerr << "WARNING: the `resources` attribute has been ignored due to its unexpected type." << std::endl;
  }

  auto txSigningAdapter = fetch::chain::TxSigningAdapterFactory(mtx);
  for (auto const& priv_key : private_keys)
  {
    mtx.Sign(priv_key, txSigningAdapter);
  }
  
  mtx.UpdateDigest();

  return mtx;
}


void handleProvidedTx(fetch::byte_array::ByteArray const &tx_jsom_string, bool const& is_verbose)
{
  if (is_verbose)
  {
    printSeparator(std::cout, "INPUT JSON");
    std::cout << tx_jsom_string << std::endl;
  }

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
      printTx(mtx, "TRANSACTION FROM PROVIDED INPUT METADATA", is_verbose); 
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
    if (args.is_verbose)
    {
      std::cout << args << std::endl;
    }

    if (args.input_json_tx_filename.empty())
    {
      auto mtx = fetch::chain::RandomTransaction(3,3);
      printTx(mtx, "RANDOM GENERATED TRANSACTION", args.is_verbose);
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
    handleProvidedTx(tx_json, args.is_verbose);

    return EXIT_SUCCESS;
  }
  catch (std::exception &ex)
  {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return EXIT_FAILURE;
}
