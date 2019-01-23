#include "mnist_loader.hpp"

#include <fstream>

using namespace std;

typedef unsigned char uchar;

uchar** read_mnist_images(string full_path, unsigned int& number_of_images, unsigned int& image_size)
{
    auto reverseInt = [](unsigned int i) -> unsigned int {
			unsigned char c1, c2, c3, c4;
        c1 = i & 255, c2 = (i >> 8) & 255, c3 = (i >> 16) & 255, c4 = (i >> 24) & 255;
        return (unsigned int)(((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4);
    };

    ifstream file(full_path, ios::binary);

    if(file.is_open()) {
      unsigned int magic_number = 0, n_rows = 0, n_cols = 0;

        file.read((char *)&magic_number, sizeof(magic_number));
        magic_number = reverseInt(magic_number);

        if(magic_number != 2051) throw runtime_error("Invalid MNIST image file!");

        file.read((char *)&number_of_images, sizeof(number_of_images)), number_of_images = reverseInt(number_of_images);
        file.read((char *)&n_rows, sizeof(n_rows)), n_rows = reverseInt(n_rows);
        file.read((char *)&n_cols, sizeof(n_cols)), n_cols = reverseInt(n_cols);

        image_size = n_rows * n_cols;

        uchar** _dataset = new uchar*[number_of_images];
        for(unsigned int i = 0; i < number_of_images; i++) {
            _dataset[i] = new uchar[image_size];
            file.read((char *)_dataset[i], std::streamsize(image_size));
        }
        return _dataset;
    } else {
        throw runtime_error("Cannot open file `" + full_path + "`!");
    }
}

uchar* read_mnist_labels(string full_path, unsigned int& number_of_labels) {
    auto reverseInt = [](unsigned int i) {
        unsigned char c1, c2, c3, c4;
        c1 = i & 255, c2 = (i >> 8) & 255, c3 = (i >> 16) & 255, c4 = (i >> 24) & 255;
        return (unsigned int)(((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4);
    };

    typedef unsigned char uchar;

    ifstream file(full_path, ios::binary);

    if(file.is_open()) {
        unsigned int magic_number = 0;
        file.read((char *)&magic_number, sizeof(magic_number));
        magic_number = reverseInt(magic_number);

        if(magic_number != 2049) throw runtime_error("Invalid MNIST label file!");

        file.read((char *)&number_of_labels, sizeof(number_of_labels)), number_of_labels = reverseInt(number_of_labels);

        uchar* _dataset = new uchar[number_of_labels];
        for(unsigned int i = 0; i < number_of_labels; i++) {
            file.read((char*)&_dataset[i], 1);
        }
        return _dataset;
    } else {
        throw runtime_error("Unable to open file `" + full_path + "`!");
    }
}

MNISTLoader::MNISTLoader()
  :cursor_(0)
{
  assert(sizeof(int) == sizeof(unsigned int));
  unsigned int recordLength(0);
  data_ = read_mnist_images("train-images-idx3-ubyte", size_, recordLength);
  assert(size_ == 60000);
  labels_ = read_mnist_labels("train-labels-idx1-ubyte", size_);
  assert(size_ == 60000);
  assert(recordLength == 28 * 28);
}

unsigned int MNISTLoader::size() const
{
  return size_;
}

bool MNISTLoader::IsDone() const
{
  return cursor_ >= size_;
}

void MNISTLoader::Reset()
{
  cursor_ = 0;
}

std::pair<unsigned int, std::shared_ptr<fetch::math::Tensor<float>>> MNISTLoader::GetNext(std::shared_ptr<fetch::math::Tensor<float>> buffer)
{
  if (!buffer)
    {
      buffer = std::make_shared<fetch::math::Tensor<float>>(std::vector<size_t>({28u, 28u}));
    }
  for (unsigned int i(0) ; i < 28*28 ; ++i)
    {
      buffer->At(i) = float(data_[cursor_][i]) / 256.0f;
    }  
  unsigned int label = (unsigned int)(labels_[cursor_]);
  cursor_++;
  return std::make_pair(label, buffer);
}

void MNISTLoader::Display(std::shared_ptr<fetch::math::Tensor<float>> const &data) const
{
  for (unsigned int j(0) ; j < 784 ; ++j)
    {
      std::cout << (data->At(j) > 0.5 ? char(219) : ' ');
      if (j % 28 == 0)
	std::cout << std::endl;
    }
}
