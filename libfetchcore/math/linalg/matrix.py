from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers

def define_interface(root, cls, selfname,  datatype):
    cls.add_method('Transpose', 'void', [  ] )
    cls.add_method('Invert', 'int', [  ] )

    cls.add_method('Sum', datatype, [  ], is_const=True )
    cls.add_method('Max', datatype, [  ], is_const=True )
    cls.add_method('Min', datatype, [  ], is_const=True )
    cls.add_method('AbsMax', datatype, [  ], is_const=True )
    cls.add_method('AbsMin', datatype, [  ], is_const=True )
    cls.add_method('Mean', datatype, [  ], is_const=True )
    cls.add_method('Dot', 
                   "void", 
                   [param(selfname +" const &", 'm'),
                    param(selfname +" &", 'ret')], 
                   is_const=True)
    cls.add_method('DotTransposed', 
                   "void", 
                   [param(selfname +" const &", 'm'),
                    param(selfname +" &", 'ret')], 
                   is_const=True)


    for op in ["+", "-", "*", "/"]:
        cls.add_inplace_numeric_operator(op+'=', param(selfname, u'other'))
        cls.add_binary_numeric_operator(op, 
                                        root[selfname], 
                                        root[selfname], root[selfname])

def build_class(root, mod, namespace, datatype):
    name =  "Matrix"
    customname = name + datatype.capitalize().replace("_t", "")

    cls = mod.add_class(name, template_parameters=[datatype], custom_name=customname)

    self_name = namespace + name+"< %s >" % datatype
    cls.add_constructor([])
    cls.add_constructor([param(self_name+' const &', 'other')])
    cls.add_constructor([param('std::size_t', 'm'), param('std::size_t', 'n')])

    define_interface(root, cls, self_name,  datatype)

    return name

import os
def build(root, mod, includedir):
    mod.add_include('"'+os.path.join(includedir, "math/linalg/matrix.hpp") +'"')

    ns = mod.add_cpp_namespace("math")
    ns = ns.add_cpp_namespace("linalg")
    build_class(root, ns, "fetch::math::linalg::", "double")
    build_class(root, ns, "fetch::math::linalg::", "float")
    build_class(root, ns, "fetch::math::linalg::", "int")
