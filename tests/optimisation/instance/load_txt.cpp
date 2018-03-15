#include<iostream>
#include<memory/rectangular_array.hpp>
#include<optimisation/instance/load_txt.hpp>
#include<random/lcg.hpp>
using namespace fetch::optimisers;
using namespace fetch::memory;
using namespace fetch::random;


void TestLoad(int argc, char **argv) {
  typedef RectangularArray<double> array_type;
  static LinearCongruentialGenerator rng;
  array_type input, output;

  if(Load(output, "some_very_very_long_non-existent_filename.yxy")) {
    std::cout << "expected load failure, but load succeeded" << std::endl;
    exit(-1);
  }
  
  std::fstream testfile(argv[1], std::ios::out);
  testfile << "# some first line" << std::endl;
  std::size_t n = (rng()>>16) & 31;
  input.Resize(n);
  for(std::size_t i=0; i < input.size(); ++i) 
    input[i] = 0 ;
  
  for(std::size_t i=0; i < n; ++i)
    for(std::size_t j=i; j < n; ++j) {
      input(i,j) = (1 -2 *rng.AsDouble()) * 100;  
      testfile << i << " " << j << " " << input(i,j) << std::endl;
    }
  
  testfile << std::flush;
  testfile.close();
  
  if(!Load(output, argv[1])) {
    std::cout << "expected load to succeed but failed" << std::endl;
    exit(-1);
  }
  
  
  
  if(input.size() != output.size()) {
    std::cout << input.size() << " " << output.size() << std::endl;
    std::cout << "input differs from output" << std::endl;
    exit(-1);
  }
  
  for(std::size_t i=0; i < input.size(); ++i) {
    if((input[i]!=0) && (fabs(input[i] -output[i]) / input[i] > 1e-5) ) {
      std::cout << i << ": " << input[i] << " vs " << output[i] << std::endl;
      exit(-1);
    }
  }
  
  }
  
  
int main(int argc, char **argv) {
  if(argc != 2) {
    std::cout << "usage: " << argv[0] << " [temp_filename]" << std::endl;
    return -1;
  }
  for(std::size_t i=0; i < 1000; ++i)
    TestLoad(argc, argv);
  return 0;
}
