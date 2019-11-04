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

# The following line should be enabled once we switch to python3.7
# from __future__ import annotations
from typing import Set, Tuple, Dict, Any, List, TypeVar, Generic  # Type, Callable
from copy import copy, deepcopy


T = TypeVar('T')


class Corners(Generic[T]):
    def __init__(self):
        self.roots: Set[T] = set()
        self.leafs: Set[T] = set()


class OrientedPath(Generic[T]):
    _ACYCLIC_INDEX: int = -1

    def __init__(self, values: List[T] = None):
        self._path: List[T] = []
        self._elements_in_path: Dict[T, int] = {}
        self._index_of_cyclic_root: int = self._ACYCLIC_INDEX
        self._hash = None

        if values is not None:
            for value in values:
                self.append(value)

    def iterate(self):
        for value in self._path:
            yield value

    def append(self, value: T):
        if not self.has(value):
            idx = len(self._path)
            self._path.append(value)
            self._elements_in_path[value] = idx
            self._hash = None
            # Acyclic
            return True

        if not self.is_cyclic:
            self._index_of_cyclic_root = self._elements_in_path[value]
            self._hash = None
            # Cyclic
            return False

        raise RuntimeError(
            "Attempting to append new node to already cyclic oriented path!")

    def pop(self) -> Tuple[bool, T]:
        if self.is_cyclic:
            cyclic = self.cyclic_node
            self._index_of_cyclic_root = self._ACYCLIC_INDEX
            self._hash = None
            return False, cyclic
        else:
            removed = self._path.pop()
            self._hash = None
            del self._elements_in_path[removed]
            return True, removed

    def has(self, value: T) -> bool:
        return value in self._elements_in_path

    @property
    def is_empty(self) -> bool:
        return len(self._path) < 1

    @property
    def root(self) -> T:
        return self.first if self.is_cyclic else None

    @property
    def leaf(self) -> T:
        return self.last if not self.is_cyclic else None

    @property
    def first(self) -> T:
        return self._path[0] if self._path else None

    @property
    def last(self) -> T:
        return self._path[-1] if self._path else None

    @property
    def is_cyclic(self) -> bool:
        return self._index_of_cyclic_root > -1

    @property
    def is_canonical_cycle(self) -> bool:
        return self._index_of_cyclic_root == 0

    @property
    def cyclic_node(self) -> T:
        return self._path[self._index_of_cyclic_root] if self.is_cyclic else None

    # def canonical_cycle_subpath(self) -> OrientedPath[T]:
    def canonical_cycle_subpath(self):
        """
        :return: OrientedPath[T]
        """

        path = None

        if self.is_cyclic:
            if self.is_canonical_cycle:
                path = self
            else:
                path = OrientedPath(self._path[self._index_of_cyclic_root:])
                path.append(self.cyclic_node)

        return path

    def __hash__(self) -> int:
        if self._hash is None:
            length = len(self._path)
            if self.is_canonical_cycle:
                # Order of hashes composition matters (we want to reflect ordering of nodes
                # the path in resulting hash).
                # However, at the same time we want to make sure that rotations of canonical cyclic
                # path gives the same hash (e.g. [1,2,3,1] gives the same hash as [3,1,2,3]).
                # To ensure both conditions above, we use `node_hash[i] = hash(tuple(path[i], path[i+1))`
                # to reflect ordering of the nodes in canonical cycle, and compose `node_hash[i]`
                # hashes together using XOR to ensure that they can be composed together
                # in any order (= starting point in canonical cycle does NOT matter) .
                ordering_hashes: List[int] = [
                    hash((self._path[i], self._path[(i+1) % length])) for i in range(0, length)]
                self._hash = 0

                # Composing resulting hash, ensuring that rotations of canonical cyclic path give identical hash
                for h in ordering_hashes:
                    self._hash = self._hash ^ h
            elif self.is_cyclic:
                # Order of hashes composition matters
                self._hash = hash(tuple(self._path) + (self.cyclic_node, ))
            else:
                # Order of hashes composition matters
                self._hash = hash(tuple(self._path))
        return self._hash

    # def __eq__(self, other: OrientedPath[T]) -> bool:
    def __eq__(self, other) -> bool:
        """
        :param other: OrientedPath[T]
        :return: bool
        """
        length = len(self._path)
        if self.is_canonical_cycle and other.is_canonical_cycle and length == len(other._path):
            right_index_shift = other._path.index(self._path[0])
            for i in range(0, length):
                left = self._path[i]
                right = other._path[(i + right_index_shift) % length]
                if left != right:
                    return False
            return True
        else:
            return self._path == other._path and self._index_of_cyclic_root == other._index_of_cyclic_root

    def __str__(self) -> str:
        return "{{{}}}: [{}]".format(("cyclic[{}]".format(str(self.cyclic_node)) if self.is_cyclic else "acyclic"),
                                     ", ".join([str(value) for value in self.iterate()]))

    @classmethod
    # def iterate_subpaths(cls, subpaths: Dict[T: List[OrientedPath[T]]]):
    def iterate_subpaths(cls, subpaths):
        """
        :param subpaths: Dict[T: List[OrientedPath[T]]]
        :return:
        """
        for root, sb in subpaths.items():
            for _path in sb:
                yield root, _path

    @classmethod
    # def iterate_subpaths(cls, subpaths: Dict[T, List[OrientedPath[T]]]) -> Dict[T: List[OrientedPath[T]]]:
    def extract_canonical_cyclic_subpaths(cls, subpaths):
        """
        :param subpaths: Dict[T, List[OrientedPath[T]]]
        :return: Set[OrientedPath[T]]
        """
        canon_cycl_subpaths: Set[OrientedPath[T]] = set()

        for root, path in OrientedPath.iterate_subpaths(subpaths):
            p: OrientedPath[T] = path.canonical_cycle_subpath()
            if p:
                canon_cycl_subpaths.add(p)

        return canon_cycl_subpaths


class Graph(Generic[T]):
    class Node:
        def __init__(self, value: T):
            self.value: T = value
            self.children: Dict[T, Graph[T].Node] = {}
            self.parents: Dict[T, Graph[T].Node] = {}

        # def add_child(self, node: Graph[T].Node):
        def add_child(self, node):
            """
            :param node: Graph[T].Node
            :return:
            """
            if not node:
                return

            if node.value not in self.children:
                self.children[node.value] = node
                node.parents[self.value] = self

        # def add_parent(self, node: Graph[T].Node):
        def add_parent(self, node):
            """
            :param node: Graph[T].Node
            :return:
            """
            if node:
                node.add_child(self)

        @property
        def is_root(self) -> bool:
            return not self.parents

        @property
        def is_leaf(self) -> bool:
            return not self.children

        def __str__(self) -> str:
            return "node:{}, children: [{}], parents: [{}]".format(self.value, ", ".join(
                [str(node.value) for node in self.children]), ", ".join([str(node.value) for node in self.parents]))

    class Roots:
        def __init__(self, roots: T):
            self.roots: T = roots

    def __init__(self):
        self.nodes: Dict[Any, Graph[T].Node] = {}

    # def _reconcile(self, corners: Corners[T], subpaths: Dict[T, List[OrientedPath[T]]],
    #               functor: Callable[[OrientedPath[T], T, bool], bool]
    #               generate_subpaths: bool):
    def _reconcile(self, corners: Corners[T],
                   subpaths: Dict[T, List[OrientedPath[T]]], functor, generate_subpaths: bool):
        """
        :param corners: Corners[T]
        :param subpaths: Dict[Any, List[OrientedPath[T]]]
        :param functor: Callable[[OrientedPath[T], T, bool], bool]
        :param generate_subpaths: bool
        :return:
        """
        cardinality_of_cyclic_roots: Dict[T, int] = {}

        for _, _path in OrientedPath.iterate_subpaths(subpaths):
            if _path.is_cyclic:
                cyclic_node_value = _path.cyclic_node

                if cyclic_node_value in corners.roots:
                    continue

                cardinality_of_cyclic_roots[cyclic_node_value] = len(
                    self.nodes[cyclic_node_value].parents)

        # Get list of cyclic roots sorted based on their cardinality (starting from highest cardinality)
        # Order matters in order to achieve the best selection of subpaths.
        cyclic_roots = sorted(cardinality_of_cyclic_roots,
                              key=cardinality_of_cyclic_roots.get, reverse=True)
        acyclic_roots = list(self.find_corners().roots)
        # Order matters - acyclic roots shall be first in order to achive best selection of subpaths.
        suggested_roots: Graph[T].Roots = Graph.Roots(
            acyclic_roots + cyclic_roots)

        return self.recurse(suggested_roots, functor, generate_subpaths)

    # def add(self, value: T, parent: Graph[T].Node=None):
    def add(self, value: T, parent=None):
        """
        :param value: T
        :param parent: Graph[T].Node
        :return:
        """
        if value in self.nodes:
            node = self.nodes[value]
        else:
            node = self.Node(value)
            self.nodes[value] = node

        if parent is None:
            return

        if parent in self.nodes:
            parent_node = self.nodes[parent]
        else:
            parent_node = Graph[T].Node(parent)
            self.nodes[parent] = parent_node

        node.add_parent(parent_node)

    # def add_graph(self, graph: Graph[T]):
    def add_graph(self, graph):
        """
        :param graph: Graph[T]
        :return:
        """
        for node_value, node in graph.nodes.items():
            self.add(value=node_value)
            for parent_value in node.parents:
                self.add(value=node_value, parent=parent_value)
            for child_value in node.children:
                self.add(value=child_value, parent=node_value)

    def add_path(self, path: OrientedPath[T]):
        prev_node_val = None
        for node_value in path.iterate():
            self.add(value=node_value, parent=prev_node_val)
            prev_node_val = node_value

        if path.is_cyclic:
            self.add(value=path.cyclic_node, parent=prev_node_val)

    def has(self, value: T):
        return value in self.nodes

    # def does_intercept_with(self, values: List[T] | Set[T]):
    def does_intercept_with(self, values):
        """
        :param values: List[T] | Set[T]
        :return: bool
        """
        values_set = values if isinstance(values, set) else set(values)
        return len(values_set.intersection(self.nodes)) > 0

    def find_corners(self) -> Corners[T]:
        corners = Corners()

        for value, node in self.nodes.items():
            if not node.children:
                corners.leafs.add(value)
            if not node.parents:
                corners.roots.add(value)

        return corners

    # def recurse(self, roots_suggestion: Set[T] | List[T] | Graph[T].Roots = None,
    #            functor: Callable[[OrientedPath[T], T, bool], bool],
    #            generate_subpaths: bool=True) -> Dict[T, List[OrientedPath[T]]]:
    def recurse(self, roots_suggestion=None, functor=None, generate_subpaths: bool = True) -> Dict[
            T, List[OrientedPath[T]]]:
        """
        :param roots_suggestion: Set[T] | List[T] | Graph[T].Roots
        :param functor: Callable[[OrientedPath[T], T, bool], bool]
        :param generate_subpaths: bool
        :return: Dict[T, List[OrientedPath[T]]]
        """
        class Context(object):
            def __init__(self, graph: Graph[T]):
                self._graph = graph

                self.unprocessed_nodes_pool: Set[T] = set()
                for node_value in self._graph.nodes:
                    self.unprocessed_nodes_pool.add(node_value)

                self._current_path = OrientedPath()
                self.subpaths: Dict[T, List[OrientedPath[T]]] = {}

            def is_cyclic(self, node: Graph[T].Node):
                return self._current_path.has(node.value)

            def finalise_path(self, current_node: Graph[T].Node, is_cyclic: bool):
                is_path = current_node.is_leaf or is_cyclic

                if not is_path:
                    return

                if not self._current_path.is_empty:
                    oriented_path = deepcopy(self._current_path)
                    root_value = oriented_path.first

                    if root_value in self.subpaths:
                        o_paths = self.subpaths[root_value]
                    else:
                        o_paths: List[OrientedPath[T]] = []
                        self.subpaths[root_value] = o_paths

                    o_paths.append(oriented_path)

            def push(self, node: Graph[T].Node):
                return self._current_path.append(node.value)

            def pop(self, node: Graph[T].Node = None):
                _, popped_node = self._current_path.pop()
                assert not node or node.value == popped_node, "Last element in the current path does not match element requested to be removed."
                if not generate_subpaths:
                    self.unprocessed_nodes_pool.discard(popped_node)

            def reduce_unprocessed_nodes_pool(self, subpaths_root: T = None):
                if not (generate_subpaths and self._current_path.is_empty):
                    return

                def reduce_pool(subpaths: List[OrientedPath[T]]):
                    for subpath in subpaths:
                        for node_value in subpath.iterate():
                            self.unprocessed_nodes_pool.discard(node_value)

                if subpaths_root is None:
                    for _, subpaths in self.subpaths.items():
                        reduce_pool(subpaths)
                else:
                    reduce_pool(self.subpaths[subpaths_root])

        def _recurse(node: Graph[T].Node, context: Context):
            """
            Recursion is done using direct language recursion mechanism,
            thus max. depth it can reach is limited by constraints defined
            by language runtime (VM).
            """

            is_path_acyclic = context.push(node)
            is_path_finalised = not is_path_acyclic or node.is_leaf
            do_continue = True
            if functor is not None and roots_suggestion is not None:
                do_continue = functor(
                    context._current_path, node.value, is_path_finalised)
            if do_continue and is_path_acyclic:
                for _, child_node in node.children.items():
                    _recurse(child_node, context)

            if generate_subpaths:
                context.finalise_path(node, not is_path_acyclic)

            context.pop(node)

        context = Context(self)

        # This *MUST* be explicitly compared against `None`, intentionally
        # *NOT* using python implicit Truthy/Falsy evaluation idiom.
        if roots_suggestion is not None:
            # If the `roots_suggestion` is of `Graph.Roots` type we rely on the order
            # of the roots in the type.
            if isinstance(roots_suggestion, Graph.Roots):
                roots = roots_suggestion.roots
            else:
                rs: Set[T] = roots_suggestion if isinstance(
                    roots_suggestion, set) else set(roots_suggestion)
                acyclic_roots = rs.intersection(self.find_corners().roots)
                cyclic_roots = rs.difference(acyclic_roots)
                # We need to make sure the acyclic roots will be processed first (order matters to achieve the best result)):
                roots = list(acyclic_roots) + list(cyclic_roots)

            for root_value in roots:
                if root_value not in context.unprocessed_nodes_pool:
                    continue
                _recurse(self.nodes[root_value], context)
                context.reduce_unprocessed_nodes_pool(subpaths_root=root_value)

            return context.subpaths

        corners = self.find_corners()
        for root_value in corners.roots:
            _recurse(self.nodes[root_value], context)
            context.reduce_unprocessed_nodes_pool()

        while context.unprocessed_nodes_pool:
            root_value = next(iter(context.unprocessed_nodes_pool))
            _recurse(self.nodes[root_value], context)
            context.reduce_unprocessed_nodes_pool(subpaths_root=root_value)

        if generate_subpaths:
            return self._reconcile(corners, context.subpaths, functor, generate_subpaths)
        else:
            return None

    def decompose_to_disjoint_subgraphs(self, subpaths: Dict[T, List[OrientedPath[T]]] = None):
        disjoint_subgraphs: List[Graph[T]] = []
        node_pool = copy(self.nodes)

        if subpaths is None:
            subpaths = self.recurse()

        subpaths_copy = copy(subpaths)

        for root_value in subpaths:
            paths = subpaths_copy[root_value]
            graph = Graph()
            for path in paths:
                graph.add_path(path)
            del subpaths_copy[root_value]

            is_joined = False
            for node_val in graph.nodes:
                if node_val in node_pool:
                    # This code-flow branch represents NON-joint graph (so far)
                    del node_pool[node_val]
                    continue

                # Following code-flow branch represents JOINT graph,
                # we need to find to witch of already exiting graphs it is joint to.
                for sg in disjoint_subgraphs:
                    if not sg.has(node_val):
                        continue
                    is_joined = True
                    sg.add_graph(graph)
                    break

                if is_joined:
                    break

            if not is_joined:
                disjoint_subgraphs.append(graph)

        return disjoint_subgraphs
