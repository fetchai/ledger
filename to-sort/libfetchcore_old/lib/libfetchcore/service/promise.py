
from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers

def define_interface(root, cls, selfname):

    cls.add_method('Wait', 'bool', [param('double', 'timeout', default_value="std::numeric_limits<double>::infinity()")])
    cls.add_method('is_fulfilled', 'bool', [])
    cls.add_method('has_failed', 'bool', [])
    cls.add_method('is_connection_closed', 'bool', [])        

    
    for name in ["uint", "int"]:
        for bits in [8,16,32,64]:
            base = name + str(bits)
            tname = base + "_t"
            cname = base.capitalize()            
            cls.add_method('As< %s >' % tname, tname, [], custom_name="As"+cname)

def build_class(root, mod, namespace):
    name =  "Promise"

    cls = mod.add_class(name)

    self_name = namespace + name
    
    cls.add_constructor([])
    define_interface(root, cls, self_name)

    return name
    
import os
def build(root, mod, includedir):
    mod.add_include('"'+os.path.join(includedir, "service/promise.hpp") +'"')

    ns = mod.add_cpp_namespace("service")
    build_class(root, ns, "fetch::service::")
