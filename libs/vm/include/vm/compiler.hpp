#ifndef COMPILER__HPP
#define COMPILER__HPP
#include "vm/parser.hpp"
#include "vm/analyser.hpp"
#include "vm/generator.hpp"


namespace fetch {
namespace vm {


class Compiler
{
public:
	Compiler() {}
	~Compiler() {}
	bool Compile(const std::string& source, const std::string& name,
		Script& script, std::vector<std::string>& errors);
	
private:

	Parser						parser_;
	Analyser					analyser;
	Generator					generator_;
};


} // namespace vm
} // namespace fetch


#endif
