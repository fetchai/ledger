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

#include "http/server.hpp"
#include <fstream>
#include <iostream>
using namespace fetch::http;

int main()
{
  fetch::network::NetworkManager tm{"NetMgr", 1};

  HTTPServer server(tm);
  server.Start(8080);

  server.AddMiddleware([](HTTPRequest &) { std::cout << "Middleware 1" << std::endl; });

  server.AddMiddleware([](HTTPResponse &res, HTTPRequest const &req) {
    std::cout << static_cast<uint16_t>(res.status()) << " " << req.uri() << std::endl;
  });

  server.AddView(Method::GET, "/", [](ViewParameters const &, HTTPRequest const &) {
    HTTPResponse res("Hello world -- this is a render of the view");

    return res;
  });

  // Add atomic to slightly slow down parallel http calls
  std::atomic<int> pages_count{0};

  server.AddView(Method::GET, "/pages",
                 [&pages_count](ViewParameters const &, HTTPRequest const &) {
                   std::ostringstream ret;
                   ret << "pages index. You have called " << pages_count++ << " times.";

                   HTTPResponse res(ret.str());

                   return res;
                 });

  server.AddView(Method::GET, "/pages/sub", [](ViewParameters const &, HTTPRequest const &) {
    HTTPResponse res("pages sub index");

    return res;
  });

  server.AddView(Method::GET, "/pages/sub/", [](ViewParameters const &, HTTPRequest const &) {
    HTTPResponse res("pages sub index with slash");

    return res;
  });

  server.AddView(Method::GET, "/pages/(id=\\d+)/", [](ViewParameters const &, HTTPRequest const &) {
    HTTPResponse res("Secret page 1");

    return res;
  });

  server.AddView(Method::GET, "/other/(name=\\w+)",
                 [](ViewParameters const &, HTTPRequest const &) {
                   HTTPResponse res("Secret page with name");

                   return res;
                 });

  server.AddView(Method::GET, "/other/(name=\\w+)/(number=\\d+)",
                 [](ViewParameters const &params, HTTPRequest const &) {
                   HTTPResponse res("Secret page with name and number: " + params["name"] +
                                    " and " + params["number"]);

                   return res;
                 });

  server.AddView(Method::GET, "/static/(filename=.+)",
                 [](ViewParameters const &params, HTTPRequest const &) {
                   std::string filename = std::string(params["filename"]);
                   std::size_t pos      = filename.find_last_of('.');
                   std::string ext      = filename.substr(pos, filename.size() - pos);
                   auto        mtype    = fetch::http::mime_types::GetMimeTypeFromExtension(ext);

                   std::cout << mtype.type << std::endl;
                   std::fstream fin(filename, std::ios::in | std::ios::binary);
                   fin.seekg(0, fin.end);
                   int64_t length = fin.tellg();
                   fin.seekg(0, fin.beg);

                   fetch::byte_array::ByteArray data;
                   data.Resize(std::size_t(length));
                   fin.read(reinterpret_cast<char *>(data.pointer()), length);

                   fin.close();

                   HTTPResponse res(data, mtype);
                   return res;
                 });

  tm.Start();

  std::cout << "HTTP server on port 8080" << std::endl;
  std::cout << "Ctrl-C to stop" << std::endl;
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  tm.Stop();

  return 0;
}
