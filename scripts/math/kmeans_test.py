import fetch
import matplotlib.pyplot as plt
import numpy as np

k = 2
n_data = 100
n_dims = 2

input_data_1 = np.random.normal(0, 1, (n_data // 2, n_dims))
input_data_2 = np.random.normal(0.1, 1, (n_data // 2, n_dims))
combined_input = np.vstack((input_data_1, input_data_2))

x = fetch.math.linalg.MatrixDouble.Zeroes([n_data, n_dims])
for i in range(n_data // 2):
    for j in range(n_dims):
        x.Set(i, j, input_data_1[i, j])
for i in range(n_data // 2):
    for j in range(n_dims):
        x.Set(i + (n_data // 2), j, input_data_2[i, j])

x.size()
output = fetch.math.clustering.KMeans(x, k)

np_output = output.ToNumpy()

print(np.shape(combined_input))
print(np.shape(combined_input[0, :]))
print(np.shape(combined_input[1, :]))

for i in range(n_data):
    if output[i] == 0:
        colour = 'b'
    else:
        colour = 'r'
    plt.scatter(combined_input[i, 0], combined_input[i, 1], color=colour)



plt.show()

