#ifndef LEXER_HPP
#define LEXER_HPP

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

bool assembly_lexer(std::string const & file_name, unsigned int & line, std::string::const_iterator & begin, std::string::const_iterator const & end, lexeme & output);

#endif
