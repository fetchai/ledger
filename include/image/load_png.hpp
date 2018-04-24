#ifndef IMAGE_LOAD_PNG_HPP
#define IMAGE_LOAD_PNG_HPP

#include "image/image.hpp"

// After http://zarb.org/~gc/html/libpng.html
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
// #define PNG_DEBUG 3
#define PNG_SKIP_SETJMP_CHECK

#include <png.h>

#include <algorithm>
#include <exception>

namespace fetch {
namespace image {

class FileReadErrorException : public std::exception {
  std::string msg_ = "";

 public:
  FileReadErrorException(std::string const &file, std::string const &msg)
      : msg_("'" + file + "': " + msg) {}
  virtual ~FileReadErrorException() throw() {}
  virtual char const *what() const throw() { return msg_.c_str(); }
};

template <typename T>
void LoadPNG(std::string const &filename, T &image) {
  typedef T image_type;
  uint32_t width, height;
  png_byte color_type;
  //  png_byte bit_depth;

  png_structp png_ptr;
  png_infop info_ptr;
  //  int number_of_passes;
  png_bytep *row_pointers;

  png_byte header[8];  // 8 is the maximum size that can be checked

  /* open file and test for it being a png */
  FILE *fp = fopen(filename.c_str(), "rb");

  if (!fp) throw FileReadErrorException(filename, "file could not be opened");
  
  if( fread(header, 1, 8, fp) != 8 * sizeof(png_byte)) {
    throw FileReadErrorException(filename,
                                 "could not read header");    
  }
  
  if (png_sig_cmp(header, (png_size_t)0, 8)) {
    throw FileReadErrorException(filename,
                                 "file was not recognized as a png file");
  }
  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
    throw FileReadErrorException(filename, "error creating png struct");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    throw FileReadErrorException(filename, "could not construct info struct");

  if (setjmp(png_jmpbuf(png_ptr)))
    throw FileReadErrorException(filename, "error while initing buffer");

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  //  bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  //  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  /* read file */
  if (setjmp(png_jmpbuf(png_ptr)))
    throw FileReadErrorException(filename, "error while reading image");

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (std::size_t y = 0; y < height; y++)
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));

  png_read_image(png_ptr, row_pointers);

  fclose(fp);

  /* Moving read stuff */
  image.Resize(height, width);

  std::size_t read_channels = 4;
  if (color_type == PNG_COLOR_TYPE_RGBA) {
    read_channels = 4;
  } else
    throw FileReadErrorException(filename, "only RGBA is currently supported");

  std::size_t common_channels =
      std::min(read_channels, std::size_t(image_type::E_CHANNELS));

  for (std::size_t i = 0; i < height; i++) {
    png_byte *row = row_pointers[i];
    for (std::size_t j = 0; j < width; j++) {
      png_byte *ptr = &(row[j * read_channels]);

      typename image_type::type val = 0;
      for (std::size_t c = common_channels; c != 0;) {
        --c;
        val <<= image_type::E_BITS_PER_CHANNEL;
        val |= ptr[c];
      }

      image.Set(i, j, val);
    }
  }
}
};
};

#endif
