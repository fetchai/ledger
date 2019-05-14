import fetch.ml.float as ml
import fetch
import sys
import os
if len(sys.argv) > 1:
    sys.path.append(sys.argv[1])
else:
    print("Usage : " + sys.argv[0] + " PATH_TO_fetch.cpython_*")
    print("Should be ../../../../build/libs/python/")
    sys.exit(0)

#            ^ change type here to work with double or fixed point

t = fetch.math.tensor.Tensor([2, 4])
t.Slice(0).Fill(-1)
t.Slice(1).Fill(1)
print(t.ToString())
relu = ml.ReluLayer()
res = relu.Forward([t])
print(res.ToString())
