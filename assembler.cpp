#include <iostream>
#include <stdexcept>
#include <vector>

#include <nil/file.hpp>

#include "parser.hpp"

int main()
{
	try
	{
		std::vector<std::string> directories;
		assembly_parser("C:\\Code\\assembler\\release\\test.asm", directories);
	}
	catch(std::exception & exception)
	{
		std::cout << "std::exception: " << exception.what() << std::endl;
	}
	return 0;
}
