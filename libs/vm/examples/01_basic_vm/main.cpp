#include "vm/compiler.hpp"
#include "vm/vm.hpp"
#include <sstream>
#include <fstream>


int main()
{

	// asserts!
	//fetch::math::linalg::Matrix<float, fetch::memory::Array<float>> a(4, 6);
	//a *= 2.7f;

	//this works
	//fetch::math::linalg::Matrix<float, fetch::memory::Array<float>> a(2, 4);
	//a *= 2.7f;

	//this works
	//fetch::math::linalg::Matrix<float, fetch::memory::Array<float>> a(4, 8);
	//a *= 2.7f;

	// this works
	//fetch::math::linalg::Matrix<float, fetch::memory::Array<float>> a(6, 12);
	//a *= 2.7f;


	// odd columns assert!
	//
	//not ok
	// fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> a(2, 3);
	// ok
	//fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> a(2, 4);
	// ok
	//fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> a(9, 6);
	// ok
	//fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> a(11, 6);
	// not ok
	//fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> a(11, 7);
	// ok
	//fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> a(11, 8);
	//a += 2.0;
	//a -= 2.0;
	//a *= 2.0;
	//a /= 2.0;


	std::ifstream file("test.txt", std::ios::binary);
	std::ostringstream ss;
	ss << file.rdbuf();
	const std::string source = ss.str();
	file.close();


	fetch::vm::Compiler* compiler = new fetch::vm::Compiler();

	fetch::vm::Script script;
	std::vector<std::string> errors;
	bool compiled = compiler->Compile(source, "myscript", script, errors);

	fetch::vm::VM vm;
	if (!compiled)
	{
          std::cout << "Failed to compile" << std::endl;
          for(auto &s: errors) {
            std::cout << s << std::endl;
          }
          return -1;
          
        }
        
        if(!script.FindFunction("main")) {
          std::cout << "Function 'main' not found" << std::endl;
          return -2;
          
        }

        vm.Execute(script, "main");
        return 0;
        
        
}
