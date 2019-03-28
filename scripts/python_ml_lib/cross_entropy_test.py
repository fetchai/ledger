import numpy as np


# x = np.array([0.1, 0.8, 0.05, 0.05])
# t = np.array([0.0, 0.1, 0.0, 0.0])                     # target probability distribution

# x = np.array([0.2, 0.5, 0.2, 0.1])
# t = np.array([0.0, 0.0, 0.1, 0.0])                     # target probability distribution

x = np.array([0.05, 0.05, 0.8, 0.1])
t = np.array([0.0, 0.0, 0.0, 0.1])                     # target probability distribution

# x = np.array([0.5, 0.1,  0.1, 0.3])
# t = np.array([0.1, 0.0, 0.0, 0.0])                     # target probability distribution


## Function definitions

def softmax(v):
    exps = np.exp(v)
    sum  = np.sum(exps)
    return exps/sum

def cross_entropy(inps,targets):
    return np.sum(-targets*np.log(inps))

def cross_entropy_derivatives(inps,targets):
    return -targets/inps

def softmax_derivatives(softmax):
    return softmax  * (1-softmax)


soft = softmax(x)                               # [0.10650698, 0.10650698, 0.78698604]
print(soft)

print(cross_entropy(soft, t))                           # 2.2395447662218846

cross_der = cross_entropy_derivatives(soft, t)   # [-0.       , -9.3890561, -0.       ]
print(cross_der)

soft_der = softmax_derivatives(soft)            # [0.09516324, 0.09516324, 0.16763901]
print(soft_der)

print(cross_der * soft_der)
print(soft - t)

# ## Derivative using chain rule
# cross_der * soft_der                            # [-0.        , -0.89349302, -0.        ]
#
#
# ## Derivative using analytical derivation
#
# soft - t                                        # [ 0.10650698, -0.89349302,  0.78698604]

