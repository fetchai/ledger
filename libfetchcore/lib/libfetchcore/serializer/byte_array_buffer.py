from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers

def define_interface(root, cls, selfname):
    for bits in [8, 16, 32, 64]:
        cls.add_method('Pack', selfname + " &", [param('uint%d_t'%bits, 'n')])
        cls.add_method('Unpack', selfname + " &", [param('uint%d_t'%bits, 'n')])
    

    cls.add_method('Seek', 'void', [param("uint64_t", "n")])    
    cls.add_method('Tell', 'uint64_t', [], is_const=True)
    cls.add_method('data',  'fetch::byte_array::ByteArray', [], is_const=True )
    
def build_class(name, root, mod, namespace):
    

    cls = mod.add_class(name)
    self_name = namespace + name    
    cls.add_constructor([])
    
    define_interface(root, cls, self_name)

    return name
    
import os
def build(root, mod, includedir):
    mod.add_include('"'+os.path.join(includedir, "serializer/stl_types.hpp") +'"')
    mod.add_include('"'+os.path.join(includedir, "serializer/byte_array_buffer.hpp") +'"')
    mod.add_include('"'+os.path.join(includedir, "serializer/typed_byte_array_buffer.hpp") +'"')
    
    ns = mod.add_cpp_namespace("serializers")
    build_class("ByteArrayBuffer",root, ns, "fetch::serializers::")
    build_class("TypedByte_ArrayBuffer",root, ns, "fetch::serializers::")
