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

#include "http/default_root_module.hpp"
#include "http/tagged_tree.hpp"

#include <map>

namespace fetch {
namespace http {

HTTPModule DefaultRootModule(MountedViews const &views)
{
  HTTPModule root;
  root.Get("/", "Returns a list of all paths available on this server.",
           [&views](ViewParameters && /*view_parameters*/, HTTPRequest && /*http_request*/) {
             static const HtmlTree header("h4", "The following paths can be queried here");

             // Sort urls by path/method
             using SortKey = std::pair<byte_array::ConstByteArray, Method>;
             std::map<SortKey, HtmlTree> known_paths;
             for (auto const &view : views)
             {
               auto path = view.route.path();
               if (path == "/")
               {
                 // this page, skip
                 continue;
               }
               // create next list item
               HtmlTree list_item("li");
               if (view.method == Method::GET && view.route.path_parameters().empty())
               {
                 // it's a GET endpoint and no parameters to bind from uri — a clickable link
                 list_item.emplace_back("a", path, HtmlParams{{"href", path}});
               }
               else
               {
                 // not a GET request, or formal parameters shown in the path, attach as plain text
                 list_item.SetContent(path);
               }
               HtmlTree row("tr");
               row.push_back(HtmlTree("td", HtmlNodes{list_item}));
               if (view.method != Method::GET)
               {
                 // display Method, for convenience and to differentiate from GET requests
                 row.push_back(HtmlTree("td", HtmlContent("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;") +
                                                  ToString(view.method)));
               }
               known_paths.emplace(SortKey{std::move(path), view.method}, std::move(row));
             }

             HtmlTree body;
             if (known_paths.empty())
             {
               body.SetContent("&lt;None&gt;");
             }
             else
             {
               body.SetTag("table");
               for (auto &known_path : known_paths)
               {
                 body.push_back(std::move(known_path.second));
               }
             }
             return HTTPResponse(HtmlBody(HtmlNodes{header, body}).Render());
           });
  return root;
}

}  // namespace http
}  // namespace fetch
