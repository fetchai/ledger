#include<iostream>
#include<image/load_png.hpp>
using namespace fetch::image;

int main(int argc, char **argv) {
  ImageRGBA img;
  LoadPNG(std::string(argv[1]), img);
  return 0;
}
