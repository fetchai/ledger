from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers


def add_type( mod, name, type ):
    cls = mod.add_class("TypedValue< %s >" % type, custom_name = name)

    cls.add_constructor([param(type, 'val')])
    cls.add_method('value', type, [])
    cls.add_method('set_value', 'void', [param(type, 'val')])
    
import os
def build(root, mod, includedir):
    mod.add_include('"'+os.path.join(includedir, "type.hpp") +'"')
    ns = mod.add_cpp_namespace("base_types")
    
    for name in ["uint", "int"]:
        for bits in [8,16,32,64]:
            base = name + str(bits)
            cname = base.capitalize()
            add_type(ns, cname, base + "_t" )

