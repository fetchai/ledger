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

#include "http/tagged_tree.hpp"

#include <numeric>

namespace fetch {
namespace http {

namespace {

/**
 * For a given tag name and a set of tag params, return tag's string representation up until
 * slash (if any), and the closing bracket, i.e.
 *
 * OpeningBracket("head", {}) --> '<head'
 * OpeningBracket("a", {{"href", "/api/status/"}}) --> '<a href="/api/status"'
 *
 * @param tag
 * @param params
 * @return
 */
HtmlTags::Content OpeningBracket(HtmlTags::Tag const &tag, HtmlTags::Params const &params)
{
  return std::accumulate(params.begin(), params.end(), "<" + tag,
                         [](HtmlTags::Content accum, HtmlTags::Params::value_type const &param) {
                           return accum + " " + param.first + "=\"" + param.second + "\"";
                         });
}

}  // namespace

HtmlTags::Content HtmlTags::operator()(Tag const &tag, Params const &params) const
{
  assert(!tag.empty());
  return OpeningBracket(tag, params) + "/>";
}

HtmlTags::Content HtmlTags::operator()(Tag const &tag, Params const &params, Content content) const
{
  // Elements can have empty tags in which case they are simply rendered in top-level space.
  // This allows to have content interleaved with child tags.
  // Example:
  //
  // HtmlTree("body",
  // 	HtmlTree::Children{
  // 		HtmlTree(top_level_content_tag, "Hello world!"),
  // 		HtmlTree("br"),
  // 		HtmlTree(top_level_content_tag, "Bye world!")
  // 	}
  // ).Render()
  //
  //    ---->
  //
  // <body>Hello world!<br/>Bye world!</body>
  if (tag.empty())
  {
    return content;
  }

  return OpeningBracket(tag, params) + ">" + content + "</" + tag + ">";
}

}  // namespace http
}  // namespace fetch
