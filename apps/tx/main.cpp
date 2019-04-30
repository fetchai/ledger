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

#define TX_SIGNING_DBG_OUTPUT

#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/commandline/params.hpp"

#include "core/json/document.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include "core/byte_array/decoders.hpp"
#include "ledger/chain/helper_functions.hpp"
#include "ledger/chain/wire_transaction.hpp"

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

using fetch::byte_array::ConstByteArray;
using fetch::variant::Extract;
using fetch::byte_array::FromBase64;

using PrivateKeys = std::set<fetch::byte_array::ConstByteArray>;

template <typename STREAM>
void PrintSeparator(STREAM &stream, std::string const &desc = std::string{})
{
  static std::string const line_separ(
      "================================================================================");
  stream << line_separ << std::endl;
  if (!desc.empty())
  {
    static std::string const prefix("====   ");
    stream << prefix << desc;

    constexpr std::size_t sfx_space_len = 3;
    std::size_t const     left_side_len = prefix.size() + desc.size();
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
  bool        is_verbose = false;

  static CommandLineArguments Parse(int argc, char **argv)
  {
    CommandLineArguments args;

    // define the parameters

    fetch::commandline::Params parameters;
    parameters.add(args.input_json_tx_filename, "f",
                   "file name for json input TX data. The json string can be provided directly as "
                   "value of this argument on command-line instead of filename.",
                   std::string{});
    parameters.add(
        args.priv_keys_filename, "p",
        "file name for private keys in json format {\"private_keys\":[\"base64_priv_key_0\"]}. Two "
        "private keys will be generated *IF* this option is *NOT* provided. The json string can be "
        "provided directly as value of this argument on command-line instead of filename. IF it is "
        "desired to disable signing (just generate Tx in wire format with NO signatures), then "
        "provide json {\"private_keys\":[]} with NO private keys as value for this parameter.",
        std::string{});
    parameters.add(args.is_verbose, "v", "enables verbose output printing out details", false);

    // parse the args
    parameters.Parse(argc, argv);

    return args;
  }

  friend std::ostream &operator<<(std::ostream &s, CommandLineArguments const &args)
  {
    PrintSeparator(s, "COMANDLINE ARGUMENTS");
    s << "input tx file          : " << args.input_json_tx_filename << std::endl;
    s << "input private keys file: " << args.priv_keys_filename << std::endl;
    s << "verbose                : " << args.is_verbose << std::endl;
    return s;
  }
};

void PrintTx(fetch::ledger::MutableTransaction const &tx, std::string const &desc = std::string{},
             bool const &is_verbose = false)
{
  if (is_verbose)
  {
    PrintSeparator(std::cout, desc);
  }
  std::cout << fetch::ledger::ToWireTransaction(tx, true) << std::endl;
}

void verifyTx(fetch::serializers::ByteArrayBuffer &tx_data_stream, bool const &is_verbose = false)
{
  fetch::ledger::MutableTransaction tx;
  auto                              txdata = fetch::ledger::TxSigningAdapterFactory(tx);
  tx_data_stream >> txdata;

  if (tx.Verify(txdata))
  {
    if (is_verbose)
    {
      PrintSeparator(std::cout, "Tx");
      std::cout << tx << std::endl;
    }
    std::cout << "SUCCESS: Transaction has been verified." << std::endl;
  }
  else
  {
    std::cerr << "FAILURE: Verification of the transaction failed!" << std::endl;
    std::cerr << tx << std::endl;
    throw std::runtime_error("FAILURE: Verification of the transaction failed!");
  }
}

fetch::ledger::MutableTransaction ConstructTxFromMetadata(fetch::variant::Variant const &metadata_v,
                                                          PrivateKeys const &private_keys)
{
  using fetch::ledger::MutableTransaction;
  using fetch::byte_array::ConstByteArray;
  using fetch::byte_array::FromBase64;

  MutableTransaction mtx;
  mtx.set_contract_name(metadata_v["contract_name"].As<ConstByteArray>());
  mtx.set_data(FromBase64(metadata_v["data"].As<ConstByteArray>()));
  mtx.set_fee(metadata_v["fee"].As<uint64_t>());

  MutableTransaction::ResourceSet resources;
  if (metadata_v.Has("resources"))
  {
    auto const &resources_v = metadata_v["resources"];

    if (resources_v.IsArray())
    {
      for (std::size_t i = 0, end = resources_v.size(); i < end; ++i)
      {
        auto const result = resources.emplace(resources_v[i].As<ConstByteArray>());
        if (!result.second)
        {
          std::cerr << "WARNING: Attempt to insert already existing resource \"" << *result.first
                    << "\"!" << std::endl;
        }
      }

      mtx.set_resources(std::move(resources));
    }
  }
  else
  {
    std::cerr << "WARNING: the `resources` attribute has been ignored due to its unexpected type."
              << std::endl;
  }

  auto txSigningAdapter = fetch::ledger::TxSigningAdapterFactory(mtx);
  for (auto const &priv_key : private_keys)
  {
    mtx.Sign(priv_key, txSigningAdapter);
  }

  mtx.UpdateDigest();

  return mtx;
}

fetch::byte_array::ConstByteArray GetJsonContentFromFileCmdlArg(std::string const &arg_value)
{
  if (arg_value.rfind("{", 0) == 0)
  {
    return arg_value;
  }

  std::ifstream istrm(arg_value, std::ios::in);
  if (!istrm.is_open())
  {
    throw std::runtime_error("File \"" + arg_value + "\" can not be opened.");
  }
  std::stringstream buffer;
  buffer << istrm.rdbuf();
  return buffer.str();
}

void HandleProvidedTx(fetch::byte_array::ConstByteArray const &tx_jsom_string,
                      PrivateKeys const &private_keys, bool const &is_verbose)
{
  if (is_verbose)
  {
    PrintSeparator(std::cout, "INPUT JSON");
    std::cout << tx_jsom_string << std::endl;
  }

  fetch::json::JSONDocument tx_json{tx_jsom_string};
  auto &                    tx_v = tx_json.root();

  ConstByteArray data;

  if (Extract(tx_v, "data", data))
  {
    fetch::serializers::ByteArrayBuffer stream{FromBase64(data)};
    verifyTx(stream, is_verbose);
  }
  else if (tx_v.Has("metadata"))
  {
    auto const &metadata_v = tx_v["metadata"];

    if (metadata_v.IsObject())
    {
      auto mtx = ConstructTxFromMetadata(metadata_v, private_keys);
      PrintTx(mtx, "TRANSACTION FROM PROVIDED INPUT METADATA", is_verbose);
    }
    else
    {
      std::cerr << "FAILURE: Input json structure does not contain neither `data` nor `metadata` "
                   "attributes."
                << std::endl;
      throw std::runtime_error(
          "FAILURE: Input json structure does not contain neither `data` nor `metadata` "
          "attributes.");
    }
  }
}

PrivateKeys GetPrivateKeys(std::string const &priv_keys_filename_argument)
{
  using SignPrivateKey = fetch::ledger::TxSigningAdapter<>::private_key_type;
  PrivateKeys keys;

  if (priv_keys_filename_argument.empty())
  {
    constexpr std::size_t num_of_keys = 2;
    for (std::size_t i = 0; i < num_of_keys; ++i)
    {
      keys.emplace(SignPrivateKey{}.KeyAsBin());
    }
    return keys;
  }

  auto priv_keys_json_string = GetJsonContentFromFileCmdlArg(priv_keys_filename_argument);
  fetch::json::JSONDocument json_doc{priv_keys_json_string};

  auto const &doc_root_v = json_doc.root();

  if (doc_root_v.Has("private_keys"))
  {
    auto const &private_keys_v = doc_root_v["private_keys"];

    if (private_keys_v.IsArray())
    {
      for (std::size_t i = 0, end = private_keys_v.size(); i < end; ++i)
      {
        auto const result = keys.emplace(FromBase64(private_keys_v[0].As<ConstByteArray>()));

        if (!result.second)
        {
          std::cerr << "WARNING: Attempt to insert already existing private key \"" << *result.first
                    << "\"!" << std::endl;
        }
      }
    }
    else
    {
      std::cerr << "WARNING: the `resources` attribute has been ignored due to its unexpected type."
                << std::endl;
    }
  }

  return keys;
}

}  // namespace

/**
 * @brief This executable generates or verifies transaction in `wire` format.
 *        Currently it's main purpose if for testing & debugging, but it can be
 *        used very well for production purposes.
 *
 * @details The app can be used in different modes:
 *    1. To **generate and sign**  random transaction (see bellow the command-line). It creates
 * random Tx data, 2 private keys, signs the transaction with them, and prints that signed
 * transaction in wire format to stdout:
 *      @code{.sh}
 *      tx-generator
 *      @endcode
 *
 *    2. To **SIGN** transaction data (`contract_name`, `fee`, `resources`, `data`). Where the
 * essential Tx data are provided in `json` format as value to command-line parameter '-f', either
 * as file-name of json file or directly as json string(if param value starts with `{` character).
 * The private keys (non mandatory) can be provided in `json` format as value to command-line
 * parameter
 * '-p', either as file-name of json file or directly as json string(if param value starts with `{`
 * character). If `-p` parameter is **not** provided, 2 private keys are generated automatically and
 * used for signing. If desired, **signing can be disabled** by providing empty list of private
 * keys. Bellow are examples of usage:
 *      @code{.sh}
 *      #1. creates wire tx from essential tx data provided in `tx_input_data.json` file and signs
 * it #    using priv. keys provided in `private_keys.json` file: tx-generator -f tx_input_data.json
 * -p private_keys.json
 *
 *      #2. creates wire tx from essential tx data provided as json string on command-line,
 *      #    and signs it using priv. keys provided as json string at command-line:
 *      tx-generator -f tx_input_data.json -p
 * '{"private_keys":["7fDTiiCsCKG43Z8YlNelveKGwir6EpCHUhrcHDbFBgg="]}'
 *
 *      #3. creates wire tx from essential tx data provided as json string on command-line,
 *      #    and signs it using priv. keys provided as json string at command-line:
 *      tx-generator -f '{"metadata": { "fee":1, "data":"YWJjZA==",
 * "resources":["aGVsbG8AACBraXR0eSwgAAAAaG93IGFyZSAAAAB5b3UwAAo=",
 * "R29vZAAAIGJ5ZSAAa2l0dHkAAAAhCg=="], "contract_name": "fetch.wealth" }}' -p
 * '{"private_keys":["7fDTiiCsCKG43Z8YlNelveKGwir6EpCHUhrcHDbFBgg="]}'
 *
 *      #4. creates wire tx from essential tx data provided in `tx_input_data.json` file and signs
 * it #    using 2 internally generated priv. keys: tx generator -f tx_input_data.json
 *
 *      #5. creates UNSIGNED wire tx from essential tx data provided as json string on command-line
 * : tx-generator -f tx_input_data.json -p '{"private_keys":[]}'
 *      @endcode
 *
 *    3. To **VERIFY** provided transaction in wire transaction (the input transaction json content
 * must be signed WIRE  Transaction, what means it MUST contain `data` element on ROOT level of the
 * json document):
 *      @code{.sh}
 *    tx-generator -f signed_wire_tx.json
 *    @endcode
 */
int main(int argc, char **argv)
{
  try
  {
    auto const args = CommandLineArguments::Parse(argc, argv);
    if (args.is_verbose)
    {
      std::cout << args << std::endl;
    }

    if (args.input_json_tx_filename.empty())
    {
      auto mtx = fetch::ledger::RandomTransaction(3, 3);
      PrintTx(mtx, "RANDOM GENERATED TRANSACTION", args.is_verbose);
      return EXIT_SUCCESS;
    }

    auto tx_json      = GetJsonContentFromFileCmdlArg(args.input_json_tx_filename);
    auto private_keys = GetPrivateKeys(args.priv_keys_filename);
    HandleProvidedTx(tx_json, private_keys, args.is_verbose);

    return EXIT_SUCCESS;
  }
  catch (std::exception const &ex)
  {
    std::cerr << "Fatal Error: " << ex.what() << std::endl;
  }

  return EXIT_FAILURE;
}
