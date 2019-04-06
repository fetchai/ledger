def sigmoid(x):
    z = x * -1.0
    z.Exp()
    y = 1.0 / (z + 1.0)
    return y


def d_sigmoid(y):
    y = y * (1.0 - y)
    return y

def relu(y, const_zeros):
    y.Maximum(const_zeros)
    return y