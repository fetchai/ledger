# ------------------------------------------------------------------------------
#
#   Copyright 2018-2019 Fetch.AI Limited
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# ------------------------------------------------------------------------------

# from __future__ import annotations
from typing import get_type_hints, Set, Tuple, Dict, List, Callable
from ..graph import Graph, Corners, OrientedPath
import unittest
from copy import deepcopy


class OrientedPathTest(unittest.TestCase):
    def test_hash_after_append_pop(self):
        p0 = OrientedPath([0, 1, 2, 3, 4, 13])

        p0.pop()
        p0.pop()
        p0.pop()
        self.assertTrue(hash(OrientedPath([0, 1, 2])) == hash(p0))

        p0.pop()
        p0.pop()
        p0.pop()
        self.assertTrue(hash(OrientedPath()) == hash(p0))

    def test_hash_equals_for_rotated_canonicaly_cyclic_path(self):
        p = OrientedPath([0, 1, 2, 3, 0])
        p_rot1 = OrientedPath([1, 2, 3, 0, 1])
        p_rot2 = OrientedPath([2, 3, 0, 1, 2])
        p_rot3 = OrientedPath([3, 0, 1, 2, 3])

        self.assertEqual(hash(p), hash(p_rot1))
        self.assertEqual(hash(p), hash(p_rot2))
        self.assertEqual(hash(p), hash(p_rot3))

        # Negative test: It is hard to implement
        # the negative proof. Reason being that
        # hash collision is not unexpected behaviour
        # (data with different value giving the same
        # hash).

    def test_comparison_equals_for_rotated_canonicaly_cyclic_path(self):
        p = OrientedPath([0, 1, 2, 3, 0])
        p_rot1 = OrientedPath([1, 2, 3, 0, 1])
        p_rot2 = OrientedPath([2, 3, 0, 1, 2])
        p_rot3 = OrientedPath([3, 0, 1, 2, 3])

        self.assertEqual(p, p_rot1)
        self.assertEqual(p, p_rot2)
        self.assertEqual(p, p_rot3)

        # Negative test/proof:
        # Proof that two canonical cyclic paths with
        # the same elements but different ordering do
        # NOT compare as equal:
        p_different = OrientedPath([1, 2, 0, 3, 1])
        self.assertNotEqual(p, p_different)


class GraphTest(unittest.TestCase):
    def test_simple_graph_iteration(self):
        g = Graph()

        # Root 0
        g.add(1, 0)
        g.add(2, 0)
        g.add(3, 1)
        g.add(4, 1)
        g.add(5, 1)
        g.add(6, 1)

        # Disjoint canonical cyclic graph
        g.add(11, 10)
        g.add(12, 11)
        g.add(11, 12)

        # Disjoint canonical cyclic graph (single node self cycle)
        g.add(13, 13)

        # Disjoint cyclic graph with roots 14 & 20 and tails 21 & 22
        # Cyclic path with root 14
        g.add_path(OrientedPath([14, 15, 16, 17, 18, 19, 15]))
        # Add root 20
        g.add(16, 20)

        # Add leaf 21
        g.add(21, 17)
        # Add leaf 22
        g.add(22, 18)

        oriented_subpaths: Set[OrientedPath[int]] = set()

        def functor(path: OrientedPath[int], node_val: int, is_finished: bool) -> bool:
            if is_finished:
                path_copy = deepcopy(path)
                oriented_subpaths.add(path_copy)
            return True

        ref_subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse(
            functor=functor, generate_subpaths=True)
        ref_subpaths_set: Set[OrientedPath[int]] = set()
        for _, path in OrientedPath.iterate_subpaths(ref_subpaths):
            ref_subpaths_set.add(path)

        diff = ref_subpaths_set.difference(oriented_subpaths)
        self.assertEqual(0, len(diff))

    def test_basic_acyclic_graph_with_roots_and_leafs(self):
        g = Graph()

        # Root 0
        g.add(1, 0)
        g.add(2, 1)
        g.add(3, 2)
        g.add(4, 3)

        # Root 5
        g.add(6, 5)
        g.add(7, 6)
        g.add(8, 7)
        g.add(4, 8)

        # Root 9
        g.add(10, 9)
        g.add(11, 10)
        g.add(12, 11)
        g.add(4, 12)

        # Add leafs
        g.add(13, 4)
        g.add(14, 4)

        subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse()

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, paths in subpaths.items():
            for path in paths:
                accum_subpaths.add(path)

        self.assertEqual(6, len(accum_subpaths))

        self.assertTrue(OrientedPath([0, 1, 2, 3, 4, 13]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 4, 14]) in accum_subpaths)
        self.assertTrue(OrientedPath([5, 6, 7, 8, 4, 13]) in accum_subpaths)
        self.assertTrue(OrientedPath([5, 6, 7, 8, 4, 14]) in accum_subpaths)
        self.assertTrue(OrientedPath([9, 10, 11, 12, 4, 13]) in accum_subpaths)
        self.assertTrue(OrientedPath([9, 10, 11, 12, 4, 14]) in accum_subpaths)

    def test_recurse_using_custom_suggested_roots(self):

        g = Graph()

        # Using directed acyclic graph for this test
        # Root 0
        g.add(1, 0)
        g.add(2, 1)
        g.add(3, 2)

        # Add extra leaf
        g.add(10, 2)

        # Root 4
        g.add(5, 4)
        g.add(6, 5)
        g.add(3, 6)

        # Add diamond tail to the graph
        g.add(7, 3)
        g.add(8, 3)
        g.add(9, 7)
        g.add(9, 8)

        # Testing scenarion #1 with root 3
        selected_roots = [3]
        subpaths: Dict[int, List[OrientedPath[int]]
                       ] = g.recurse(selected_roots)

        self.assertEqual(len(selected_roots), len(subpaths))
        self.assertEqual(set(selected_roots), subpaths.keys())

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, path in OrientedPath.iterate_subpaths(subpaths):
            accum_subpaths.add(path)

        self.assertEqual(2, len(accum_subpaths))

        self.assertTrue(OrientedPath([3, 7, 9]) in accum_subpaths)
        self.assertTrue(OrientedPath([3, 8, 9]) in accum_subpaths)

        # Testing scenario #2 with root 0
        selected_roots = [0]
        subpaths: Dict[int, List[OrientedPath[int]]
                       ] = g.recurse(selected_roots)

        self.assertEqual(len(selected_roots), len(subpaths))
        self.assertEqual(set(selected_roots), subpaths.keys())

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, path in OrientedPath.iterate_subpaths(subpaths):
            accum_subpaths.add(path)

        self.assertEqual(3, len(accum_subpaths))

        self.assertTrue(OrientedPath([0, 1, 2, 10]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 7, 9]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 8, 9]) in accum_subpaths)

        # Testing scenario #3 with roots 0 and 3, which should give the SAME
        # result as the scenario #2 because root 3 lies on the path(s) seeded
        # from the root 0
        selected_roots = [0, 3]
        subpaths: Dict[int, List[OrientedPath[int]]
                       ] = g.recurse(selected_roots)

        # We expect only one root
        self.assertEqual(1, len(subpaths))
        # The expected root
        self.assertTrue(0 in subpaths)

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, path in OrientedPath.iterate_subpaths(subpaths):
            accum_subpaths.add(path)

        self.assertEqual(3, len(accum_subpaths))

        self.assertTrue(OrientedPath([0, 1, 2, 10]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 7, 9]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 8, 9]) in accum_subpaths)

        # Testing scenario #4 with roots 0 and 4, which shall cover the whole
        # graph (all paths in the whole graph) because the this is acyclic graph,
        # and 0 and 4 are actually all roots of the graph
        selected_roots = [0, 4]
        subpaths: Dict[int, List[OrientedPath[int]]
                       ] = g.recurse(selected_roots)

        self.assertEqual(len(selected_roots), len(subpaths))
        self.assertEqual(set(selected_roots), subpaths.keys())

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, path in OrientedPath.iterate_subpaths(subpaths):
            accum_subpaths.add(path)

        self.assertEqual(5, len(accum_subpaths))

        self.assertTrue(OrientedPath([0, 1, 2, 10]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 7, 9]) in accum_subpaths)
        self.assertTrue(OrientedPath([0, 1, 2, 3, 8, 9]) in accum_subpaths)
        self.assertTrue(OrientedPath([4, 5, 6, 3, 7, 9]) in accum_subpaths)
        self.assertTrue(OrientedPath([4, 5, 6, 3, 8, 9]) in accum_subpaths)

        subpaths_inferred: Dict[int, List[OrientedPath[int]]] = g.recurse()

        # As mentioned above roots 0 and 4 must cover all graph, thus recursing tree
        # from them must cover whole tree, thus the result must be the same as the one
        # produced by automatic inferring:
        self.assertEqual(subpaths, subpaths_inferred)

    def test_recurse_reult_is_invariant_to_order_of_custom_suggested_roots(self):
        g = Graph()

        # Using directed acyclic graph for this test
        # Root 0
        g.add(1, 0)
        g.add(2, 1)
        g.add(3, 2)

        # Add extra leaf
        g.add(10, 2)

        # Root 4
        g.add(5, 4)
        g.add(6, 5)
        g.add(3, 6)

        # Add diamond tail to the graph
        g.add(7, 3)
        g.add(8, 3)
        g.add(9, 7)
        g.add(9, 8)

        # Recurse graph twice, each time with list of same roots but
        # in different order.
        subpaths_0_3 = g.recurse([0, 3])
        subpaths_3_0 = g.recurse([3, 0])

        self.assertEqual(subpaths_0_3, subpaths_3_0)

    def test_joint_canonical_cycles(self):
        g = Graph()

        # Canonical cycle
        g.add(1, 0)
        g.add(2, 1)
        g.add(3, 2)
        g.add(4, 3)
        g.add(0, 4)

        # Canonical cycle (joint graph to previous via node 2)
        g.add(5, 2)
        g.add(6, 5)
        g.add(7, 6)
        g.add(8, 7)
        g.add(2, 8)

        subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse()

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, paths in subpaths.items():
            for path in paths:
                accum_subpaths.add(path)

        self.assertEqual(2, len(accum_subpaths))

        self.assertTrue(OrientedPath([2, 3, 4, 0, 1, 2]) in accum_subpaths)
        self.assertTrue(OrientedPath([2, 5, 6, 7, 8, 2]) in accum_subpaths)

    def test_joint_canonical_cycles_2(self):
        g = Graph()

        # Starting with Canonical cycle #1
        g.add(0)
        g.add(1, 0)
        g.add(0, 1)

        # Appended cycle #2
        g.add(2, 1)
        g.add(1, 2)

        # Appended cycle #3
        g.add(3, 2)
        g.add(2, 3)

        subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse()

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, paths in subpaths.items():
            for path in paths:
                accum_subpaths.add(path)

        self.assertEqual(3, len(accum_subpaths))

        # Since we have 3 canonically cyclic graphs joined in to single graph
        # we expect only *SINGLE* root to be present, based on cyclic cardinality
        # it must be either 1 or 2. They have identical cyclic cardinality,
        # thus we need to detect which one was selected on this platform.
        self.assertEqual(1, len(subpaths))

        cyclic_root = list(subpaths.keys())[0]

        if cyclic_root == 1:
            self.assertTrue(OrientedPath([1, 0, 1]) in accum_subpaths)
            self.assertTrue(OrientedPath([1, 2, 1]) in accum_subpaths)
            self.assertTrue(OrientedPath([1, 2, 3, 2]) in accum_subpaths)
        elif cyclic_root == 2:
            self.assertTrue(OrientedPath([2, 3, 2]) in accum_subpaths)
            self.assertTrue(OrientedPath([2, 1, 2]) in accum_subpaths)
            self.assertTrue(OrientedPath([2, 1, 0, 1]) in accum_subpaths)
        else:
            self.assertTrue(False)

    def test_basic_cyclic_graph_with_roots_and_leafs(self):
        g = Graph()

        # Canonical cycle
        g.add(1, 0)
        g.add(2, 1)
        g.add(3, 2)
        g.add(4, 3)
        g.add(0, 4)

        # Add Root 5
        g.add(0, 5)

        # Add Root 6
        g.add(4, 6)

        # Add leaf 7
        g.add(7, 2)

        # Add leaf 8
        g.add(8, 3)

        subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse()

        accum_subpaths: Set[OrientedPath[int]] = set()
        for _, path in OrientedPath.iterate_subpaths(subpaths):
            accum_subpaths.add(path)

        self.assertEqual(6, len(accum_subpaths))

        self.assertTrue(OrientedPath([5, 0, 1, 2, 7]) in accum_subpaths)
        self.assertTrue(OrientedPath([5, 0, 1, 2, 3, 8]) in accum_subpaths)
        self.assertTrue(OrientedPath([5, 0, 1, 2, 3, 4, 0]) in accum_subpaths)
        self.assertTrue(OrientedPath([6, 4, 0, 1, 2, 7]) in accum_subpaths)
        self.assertTrue(OrientedPath([6, 4, 0, 1, 2, 3, 8]) in accum_subpaths)
        self.assertTrue(OrientedPath([6, 4, 0, 1, 2, 3, 4]) in accum_subpaths)

    def test_extract_canonycal_cyclic_subpaths(self):
        g = Graph()

        # Joint Cycle #1
        g.add(3, 2)
        g.add(4, 3)
        g.add(2, 4)
        # Adding roots
        g.add(0, 2)
        g.add(1, 3)
        # Adding leafs
        g.add(3, 20)
        g.add(4, 21)

        # Joint Cycle #2
        g.add(5, 2)
        g.add(6, 5)
        g.add(2, 6)
        # Adding leafs
        g.add(5, 22)
        g.add(6, 23)

        # Joit Cycle #3
        g.add(7, 3)
        g.add(8, 7)
        g.add(3, 8)
        # Adding root
        g.add(24, 7)
        # Adding leaf
        g.add(8, 25)

        # Disjoit Canonical Cycle #4
        g.add(10, 9)
        g.add(11, 10)
        g.add(9, 10)

        subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse()
        canonical_cyclic_subpats: List[OrientedPath[int]
                                       ] = OrientedPath.extract_canonical_cyclic_subpaths(subpaths)

        self.assertEqual(4, len(canonical_cyclic_subpats))

    def test_graph_decomposition_to_disjoint_subgraphs(self):
        # Subgraph composed of TWO joint canonical cycles, but disjoint from other subgraphs
        g1 = Graph()
        g1.add(1, 0)
        g1.add(0, 1)
        g1.add(2, 1)
        g1.add(1, 2)

        # Linear subgraph, disjoint from other subgraphs
        g2 = Graph()
        g2.add(4, 3)
        g2.add(5, 4)
        g2.add(6, 5)

        # Acyclic subgraph, disjoint from other subgraphs
        g3 = Graph()
        g3.add(8, 7)
        g3.add(9, 7)
        g3.add(10, 7)
        g3.add(11, 8)
        g3.add(11, 9)
        g3.add(12, 9)
        g3.add(12, 10)
        g3.add(13, 11)
        g3.add(13, 12)

        # Compose/Join subgraphs in to single super-graph
        g = Graph()
        g.add_graph(g1)
        g.add_graph(g2)
        g.add_graph(g3)

        # Just for consistency we check basics in joint single super-graph
        subpaths: Dict[int, List[OrientedPath[int]]] = g.recurse()
        # We expect 3 roots = [1, 3, 7]
        self.assertEqual(3, len(subpaths))
        self.assertEqual(set([1, 3, 7]), subpaths.keys())

        disjoint_subgraphs: List[Graph] = g.decompose_to_disjoint_subgraphs()

        # We expect 3 disjoint subgraphs
        self.assertEqual(3, len(disjoint_subgraphs))

        subpaths_from_disjoint_subgraphs: Dict[int,
                                               List[OrientedPath[int]]] = {}
        for disj_subgraph in disjoint_subgraphs:
            disj_subgraph_subpaths = disj_subgraph.recurse()
            subpaths_from_disjoint_subgraphs.update(disj_subgraph_subpaths)

        self.assertEqual(subpaths, subpaths_from_disjoint_subgraphs)
