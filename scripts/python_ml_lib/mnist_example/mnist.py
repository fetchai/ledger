import os
import gzip
import numpy as np

DATA_URL = 'http://yann.lecun.com/exdb/mnist/'


def load_data(one_hot=True, reshape=None,
              training_size=10000, validation_size=50):
    """Download and import the MNIST dataset from Yann LeCun's website.

    Reserve 10,000 examples from the training set for validation.
    Each image is an array of 784 (28x28) float values  from 0 (white) to 1 (black).
    """
    x_tr = load_images('train-images-idx3-ubyte.gz')
    y_tr = load_labels('train-labels-idx1-ubyte.gz')
    x_te = load_images('t10k-images-idx3-ubyte.gz')
    y_te = load_labels('t10k-labels-idx1-ubyte.gz')

    if training_size is None:
        x_tr = x_tr[:-validation_size]
        y_tr = y_tr[:-validation_size]
    else:
        x_tr = x_tr[:training_size]
        y_tr = y_tr[:training_size]

    if validation_size is None:
        x_te = x_te[:validation_size]
        y_te = y_te[:validation_size]
    else:
        x_te = x_te[:validation_size]
        y_te = y_te[:validation_size]

    if one_hot:
        y_tr, y_te = [to_one_hot(y) for y in (y_tr, y_te)]

    if reshape:
        x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]

    return x_tr, y_tr, x_te, y_te


def load_images(filename):
    maybe_download(filename)
    with gzip.open(filename, 'rb') as f:
        data = np.frombuffer(f.read(), np.uint8, offset=16)
    return data.reshape(-1, 28 * 28) / np.float32(256)


def load_labels(filename):
    maybe_download(filename)
    with gzip.open(filename, 'rb') as f:
        data = np.frombuffer(f.read(), np.uint8, offset=8)
    return data


def maybe_download(filename):
    """Download the file, unless it's already here."""
    if not os.path.exists(filename):
        from urllib.request import urlretrieve
        print("Downloading %s" % filename)
        urlretrieve(DATA_URL + filename, filename)


def to_one_hot(labels, num_classes=10):
    """Convert class labels from scalars to one-hot vectors."""
    return np.eye(num_classes)[labels]
