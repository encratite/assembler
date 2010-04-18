#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <string>
#include <vector>

enum lexeme_type
{
	lexeme_name,
	lexeme_number,
	lexeme_string,
	lexeme_operator,
	lexeme_newline
};

struct lexeme
{
	lexeme_type type;
	std::string data;
	unsigned int line;

	lexeme();
	lexeme(lexeme_type type, unsigned int line);
	lexeme(lexeme_type type, std::string const & data, unsigned int line);
};

#endif
