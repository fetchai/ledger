from .. import libfetchcore
from .. import types

class Matrix(object):
    matrix_ = None
    type_ = None

    def __init__(self, *args, **kwargs):
        h = 0
        w = 0

        if len(args) == 1:
           h = args[0]
           w = 1
        elif len(args) == 2:
           h = args[0]
           w = args[1]
        else:
           if "height" in kwargs: h = kwargs["height"]
           if "width" in kwargs: w = kwargs["width"]

        type = types.double
        if "type" in kwargs: type = kwargs["type"]
        cls = None
        if type == types.double:
            cls = libfetchcore.fetch.math.linalg.MatrixDouble
        elif type == types.float:
            cls = libfetchcore.fetch.math.linalg.MatrixFloat
        elif type == types.int:
            cls = libfetchcore.fetch.math.linalg.MatrixInt
        else:
            raise BaseException("Unknown type %s" % str(type))
        
        self.matrix_ = cls()



    def __getitem__(self, i):
        return self.matrix_.At( i )

    def __getitem__(self, index):
        return self.matrix_.At( index[0], index[1] )

    def __setitem__(self,  i,  v):
        self.matrix_.Set( i, v )

    def __setitem__(self, index, v):
        self.matrix_.Set( index[0], index[1], v )

    def Min(self):
        return self.matrix_.Min()

    def Max(self):
        return self.matrix_.Max()

    def AbsMin(self):
        return self.matrix_.AbsMin()

    def AbsMax(self):
        return self.matrix_.AbsMax()

    def Mean(self):
        return self.matrix_.Mean()
    
def AsNPArray(mat):
    ret = numpy.zeros((mat.height(), mat.width()))
    for i in range(mat.height()):
        for j in range(mat.width()):
            ret[i,j] = mat.matrix_.At(i,j)
    return ret

def FromNPArray(arr):
    height, width = arr.shape
    mat = Matrix(height, width)
 
    for i in range(height):
        for j in range(width):
            mat.matrix_.Set(i,j, ret[i,j])
            
    return mat
