import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'build', 'libs', 'python')) 
import fetch

t = fetch.math.tensor.Tensor([5, 4])
print(t.Size())
print(t.ToString())
t.Fill(42)
print(t.ToString())

t2 = t.Slice(3)
t2.Fill(24)

print(t.ToString())

t.Set([0, 0], 101)
t.Set(2, 999)

for i in range(0, t.Size()):
    print(t.At(i))
