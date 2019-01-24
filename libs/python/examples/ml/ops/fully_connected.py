import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..', 'build', 'libs', 'python')) 
import fetch

t = fetch.math.tensor.Tensor([2, 4])
t.Slice(0).Fill(-1)
t.Slice(1).Fill(1)
print(t.ToString())
fc = fetch.ml.FullyConnected(8, 12)
res = fc.Forward([t])
print(res.ToString())
