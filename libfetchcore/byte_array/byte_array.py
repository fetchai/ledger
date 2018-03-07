
from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers

def define_interface(root, cls, selfname):

    cls.add_method('Resize', 'void', [param('uint64_t', 'n')])
    cls.add_method('Reserve', 'void', [param('uint64_t', 'n')])

    cls.add_binary_numeric_operator("+", 
                                    root[selfname], 
                                    root[selfname], root[selfname])
    cls.add_binary_comparison_operator("<")

    cls.add_binary_comparison_operator("==")
    cls.add_binary_comparison_operator("!=")


    cls.add_method('capacity', 'uint64_t', [], is_const=True)
    cls.add_method('SubArray', selfname, [param('uint64_t', 'n'), param('uint64_t', 'length')], is_const=True)
    cls.add_method('Match', 'bool', [param(selfname +" const &", 'n'), param( 'uint64_t', 'pos', default_value="0")], is_const=True)
    cls.add_method('Find', 'bool', [param("char", 'c'), param( 'uint64_t', 'pos', default_value="0")], is_const=True)    
    cls.add_method('size', 'uint64_t', [], is_const=True)

def build_class(root, mod, namespace):
    name =  "ByteArray"

    cls = mod.add_class(name)

    self_name = namespace + name
    
    cls.add_constructor([])
    cls.add_constructor([param(self_name+' const &', 'other')])
    cls.add_constructor([param('std::string const &', 'other')])    

    
    define_interface(root, cls, self_name)

    return name
    
import os
def build(root, mod, includedir):
    mod.add_include('"'+os.path.join(includedir, "byte_array/referenced_byte_array.hpp") +'"')

    ns = mod.add_cpp_namespace("byte_array")
    build_class(root, ns, "fetch::byte_array::")
