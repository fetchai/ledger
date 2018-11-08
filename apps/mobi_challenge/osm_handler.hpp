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

#pragma once

#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/visitor.hpp>

struct GraphHandler : public osmium::handler::Handler
{

  //  Graph graph;
  //
  //  // TODO there should be a way to have a map directly into the Boost graph.
  //  std::map<uint64_t,  Graph::vertex_descriptor> osmNodeId_to_vertexDescriptor;
  //  std::map<uint64_t,  OSMNode> osmNodeId_to_osmNode;

  // This callback is called by osmium::apply for each node in the data.
  void node(const osmium::Node &node) noexcept
  {
    //    auto id = (uint64_t) node.id();
    //    GPS coordinates{node.location().lon(), node.location().lat()};
    //    OSMNode osm_node{id, coordinates._lon, coordinates._lat};
    //
    //    Graph::vertex_descriptor v = boost::add_vertex(graph);
    //    osmNodeId_to_vertexDescriptor[id] = v;
    //    osmNodeId_to_osmNode[id] = osm_node;
    //
    ////        std::cout << id << " " << v << " " << std::endl;
  }

  // This callback is called by osmium::apply for each way in the data.
  void way(const osmium::Way &way) noexcept
  {
    //    auto const& nodes = way.nodes();
    //
    //    auto u = osmNodeId_to_vertexDescriptor[nodes.begin()->ref()];
    ////        std::cout << "Start node id: " << nodes.begin()->ref() << std::endl;
    //
    //    for(unsigned int i = 1; i < nodes.size(); i++){
    //      auto v = osmNodeId_to_vertexDescriptor[nodes[i].ref()];
    //      boost::add_edge(u, v, graph);
    ////            std::cout << boost::format("u=%d\tv=%d") % u % v << std::endl;
    //      u = v;
    //    }
  }
};

inline GraphHandler parse_map_with_osmium(std::string &filepath)
{

  osmium::io::File   input_file{filepath.c_str()};
  osmium::io::Reader reader{input_file};

  GraphHandler handler;
  osmium::apply(reader, handler);

  reader.close();

  //  std::cout << "Nodes: "     << handler.graph.vertex_set().size() << "\n";
  //  std::cout << "Edges: "     << handler.graph.m_edges.size() << "\n";

  osmium::MemoryUsage memory;
  //  std::cout << "\nMemory used: " << memory.peak() << " MBytes\n";

  return handler;
}