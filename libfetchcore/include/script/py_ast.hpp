#ifndef LIBFETCHCORE_SCRIPT_AST_HPP
#define LIBFETCHCORE_SCRIPT_AST_HPP
#include "script/ast.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace script
{

void BuildAbstractSyntaxTree(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<AbstractSyntaxTree>(module, "AbstractSyntaxTree" )
    .def(py::init<>()) /* No constructors found */
    .def("PushClose", &AbstractSyntaxTree::PushClose)
    .def("PushToken", &AbstractSyntaxTree::PushToken)
    .def("AddLeftRight", &AbstractSyntaxTree::AddLeftRight)
    .def("tree", &AbstractSyntaxTree::tree)
    .def("Clear", &AbstractSyntaxTree::Clear)
    .def("AddOperation", &AbstractSyntaxTree::AddOperation)
    .def("PushTokenType", &AbstractSyntaxTree::PushTokenType)
    .def("Push", &AbstractSyntaxTree::Push)
    .def("PushOpen", &AbstractSyntaxTree::PushOpen)
    .def("AddRight", &AbstractSyntaxTree::AddRight)
    .def("AddGroup", &AbstractSyntaxTree::AddGroup)
    .def("root_shared_pointer", ( const fetch::script::AbstractSyntaxTree::ast_node_ptr & (AbstractSyntaxTree::*)() const ) &AbstractSyntaxTree::root_shared_pointer)
    .def("root_shared_pointer", ( fetch::script::AbstractSyntaxTree::ast_node_ptr & (AbstractSyntaxTree::*)() ) &AbstractSyntaxTree::root_shared_pointer)
    .def("AddToken", &AbstractSyntaxTree::AddToken)
    .def("AddLeft", &AbstractSyntaxTree::AddLeft)
    .def("root", ( const fetch::script::AbstractSyntaxTree::node_type & (AbstractSyntaxTree::*)() const ) &AbstractSyntaxTree::root)
    .def("root", ( fetch::script::AbstractSyntaxTree::node_type & (AbstractSyntaxTree::*)() ) &AbstractSyntaxTree::root)
    .def("Build", &AbstractSyntaxTree::Build);

}
};
};

#endif