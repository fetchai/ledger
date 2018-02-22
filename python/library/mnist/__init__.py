import os
import gzip
import numpy as np

DATA_URL = 'http://yann.lecun.com/exdb/mnist/'
DIR = os.path.abspath(os.path.dirname(__file__))
def load_data(one_hot=True, reshape=None, validation_size=10000):
    x_tr = load_images('train-images-idx3-ubyte.gz')
    y_tr = load_labels('train-labels-idx1-ubyte.gz')
    x_te = load_images('t10k-images-idx3-ubyte.gz')
    y_te = load_labels('t10k-labels-idx1-ubyte.gz')

    x_tr = x_tr[:-validation_size]
    y_tr = y_tr[:-validation_size]

    if one_hot:
        y_tr, y_te = [to_one_hot(y) for y in (y_tr, y_te)]

    if reshape:
        x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]

    return x_tr, y_tr, x_te, y_te

def load_images(filename):
    with gzip.open(os.path.join(DIR, filename), 'rb') as f:
        data = np.frombuffer(f.read(), np.uint8, offset=16)
    return data.reshape(-1, 28 * 28) / np.float32(256)

def load_labels(filename):
    with gzip.open(os.path.join(DIR, filename), 'rb') as f:
        data = np.frombuffer(f.read(), np.uint8, offset=8)
    return data

def to_one_hot(labels, num_classes=10):
    return np.eye(num_classes)[labels]
