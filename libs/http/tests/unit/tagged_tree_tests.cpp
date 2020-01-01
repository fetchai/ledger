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
#include "http/tagged_tree.hpp"
#include "network/fetch_asio.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <ostream>

using fetch::http::HtmlTree;
using fetch::http::HtmlNodes;
using Content = HtmlTree::Content;
using Params  = HtmlTree::Params;

TEST(HtmlTreeTests, SingletonTag)
{
  ASSERT_EQ(HtmlTree("hello").Render(), Content("<hello/>"));
  ASSERT_EQ(HtmlTree("hello", "").Render(), Content("<hello/>"));
  ASSERT_EQ(HtmlTree("hello", Params{{"location", "world"}, {"answer", "42"}}).Render(),
            Content("<hello location=\"world\" answer=\"42\"/>"));
}

TEST(HtmlTreeTests, PlainTextContent)
{
  ASSERT_EQ(HtmlTree("", "hello").Render(), Content("hello"));
  ASSERT_EQ(HtmlTree(fetch::http::top_level_content, "hello").Render(), Content("hello"));
}

TEST(HtmlTreeTests, SimpleNode)
{
  ASSERT_EQ(HtmlTree("hello", "world").Render(), Content("<hello>world</hello>"));
  ASSERT_EQ(HtmlTree("hello", "world", Params{{"location", "world"}, {"answer", "42"}}).Render(),
            Content("<hello location=\"world\" answer=\"42\">world</hello>"));
}

TEST(HtmlTreeTests, Children)
{
  HtmlTree large("hello",
                 HtmlNodes{HtmlTree("singleton-subtag"), HtmlTree("", "more top level content"),
                           HtmlTree("simple-subtag", "with content"),
                           HtmlTree("complex-subtag", "with content", {HtmlTree("and-more")})});
  ASSERT_EQ(large.Render(), Content("<hello>"
                                    "<singleton-subtag/>"
                                    "more top level content"
                                    "<simple-subtag>with content</simple-subtag>"
                                    "<complex-subtag><and-more/>with content</complex-subtag>"
                                    "</hello>"));

  large = HtmlTree("hello",
                   HtmlNodes{HtmlTree("singleton-subtag"), HtmlTree("", "more top level content"),
                             HtmlTree("simple-subtag", "with content"),
                             HtmlTree("complex-subtag", "with content", {HtmlTree("and-more")})},
                   Params{{"location", "world"}, {"answer", "42"}});
  ASSERT_EQ(large.Render(), Content("<hello location=\"world\" answer=\"42\">"
                                    "<singleton-subtag/>"
                                    "more top level content"
                                    "<simple-subtag>with content</simple-subtag>"
                                    "<complex-subtag><and-more/>with content</complex-subtag>"
                                    "</hello>"));

  HtmlTree huge("hello", "top level content",
                HtmlNodes{HtmlTree("singleton-subtag"),
                          HtmlTree(fetch::http::top_level_content, "more top level content"),
                          HtmlTree("simple-subtag", "with content"),
                          HtmlTree("complex-subtag", "with content", {HtmlTree("and-more")})});
  ASSERT_EQ(huge.Render(), Content("<hello>"
                                   "<singleton-subtag/>"
                                   "more top level content"
                                   "<simple-subtag>with content</simple-subtag>"
                                   "<complex-subtag><and-more/>with content</complex-subtag>"
                                   "top level content"
                                   "</hello>"));

  huge = HtmlTree("hello", "top level content",
                  HtmlNodes{HtmlTree("singleton-subtag"),
                            HtmlTree(fetch::http::top_level_content, "more top level content"),
                            HtmlTree("simple-subtag", "with content"),
                            HtmlTree("complex-subtag", "with content", {HtmlTree("and-more")})},
                  Params{{"location", "world"}, {"answer", "42"}});
  ASSERT_EQ(huge.Render(), Content("<hello location=\"world\" answer=\"42\">"
                                   "<singleton-subtag/>"
                                   "more top level content"
                                   "<simple-subtag>with content</simple-subtag>"
                                   "<complex-subtag><and-more/>with content</complex-subtag>"
                                   "top level content"
                                   "</hello>"));
}
