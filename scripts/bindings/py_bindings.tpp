#ifndef LIBFETCHCORE_{{ HEADER_GUARD }}
#define LIBFETCHCORE_{{ HEADER_GUARD }}
{%- for i in includes %}
#include "{{i}}"
{%- endfor %}

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
{%- for space in exporting.itervalues() -%}
{%- set namespace = space.namespace -%}
{%- for n in namespace %}
namespace {{n}}
{
{%- endfor -%}

{% for class in space.classes %}
{% set classname = class.classname %}
{% set constructors = class.constructors -%}
{%- set custom_name = class.custom_name -%}
{%- set methods = class.methods -%}

{%if class.is_template -%}
template< {{ class.short_template_param_definition }} >
void Build{{classname}}(std::string const &custom_name, pybind11::module &module) {
{% set custom_name = "custom_name" -%}
{%- else -%}
void Build{{classname}}(pybind11::module &module) {
{%- set custom_name = "\""+custom_name+"\"" -%}
{%- endif %}
  namespace py = pybind11;
  {% set params = [class.qualified_class_name] + class.bases -%}

  py::class_<{{", ".join(params)}}>(module, {{custom_name}} )
    {%- if len(constructors) == 0 %}
    .def(py::init<>()) /* No constructors found */
    {%- else -%}
    {%- for c in constructors %}
    .def(py::init< {{ ", ".join(c.arguments) }} >())
    {%- endfor -%}
    {%- endif -%}
  {%- for m in methods.itervalues() -%}

  {%- if m.is_operator %}
  {%- for d in m.details %}
    .def({{d.op_args[0]}} {{m.op_name}} {{d.op_args[1]}} )
  {%- endfor -%}
  {%- elif len(m.details) == 1 %}
    .def("{{m.name}}", &{{ class.qualified_class_name }}::{{m.name}})
  {%- else -%}
    {%- for d in m.details %}
    .def("{{m.name}}", ( {{d.full_signature}} ) &{{ class.qualified_class_name }}::{{m.name}})
    {%- endfor -%}
  {% endif -%}
  {%- endfor -%};

}

{%- endfor -%}


{%- for n in namespace %}
};
{%- endfor -%}


{%- endfor %}

#endif
