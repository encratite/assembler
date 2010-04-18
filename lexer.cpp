#include "lexer.hpp"

#include <nil/string.hpp>
#include <nil/prepare.hpp>

namespace
{
	std::size_t const alphabet_size = 256;

	char const
		char_type_illegal = 0,
		char_type_name = 1,
		char_type_digit = 2,
		char_type_zero = 3,
		char_type_string = 4,
		char_type_operator = 5,
		char_type_operator_extended = 6,
		char_type_whitespace = 7,
		char_type_newline = 8,
		char_type_comment = 9;

	char
		type_lookup_table[alphabet_size],
		is_name_char_lookup_table[alphabet_size];

	void set_lookup_table_range(char * lookup_table, char first, char last, char value = 1)
	{
		for(; first <= last; ++first)
		{
			lookup_table[static_cast<std::size_t>(first)] = value;
		}
	}

	void set_lookup_table_string(char * lookup_table, std::string const & input, char value)
	{
		for(std::string::const_iterator i = input.begin(), end = input.end(); i != end; ++i)
		{
			lookup_table[static_cast<std::size_t>(*i)] = value;
		}
	}

	void initialise_lookup_tables()
	{
		set_lookup_table_range(type_lookup_table, 'a', 'z', char_type_name);
		set_lookup_table_range(type_lookup_table, 'A', 'Z', char_type_name);
		type_lookup_table['_'] = char_type_name;

		set_lookup_table_range(type_lookup_table, '1', '9', char_type_digit);
		type_lookup_table['0'] = char_type_zero;

		type_lookup_table['"'] = char_type_string;

		set_lookup_table_string(type_lookup_table, ",:[]()+-*/^~", char_type_operator);
		set_lookup_table_string(type_lookup_table, "&|=!<>", char_type_operator_extended);
		set_lookup_table_string(type_lookup_table, " \r\t", char_type_whitespace);

		type_lookup_table['\n'] = char_type_newline;

		type_lookup_table[';'] = char_type_comment;

		set_lookup_table_range(is_name_char_lookup_table, 'a', 'z');
		set_lookup_table_range(is_name_char_lookup_table, 'A', 'Z');
		set_lookup_table_range(is_name_char_lookup_table, '0', '9');
		is_name_char_lookup_table['_'] = 1;
	}

	nil::prepare prepare_lookup_tables(&initialise_lookup_tables);
}

lexeme::lexeme()
{
}

lexeme::lexeme(lexeme_type type, unsigned int line):
	type(type),
	line(line)
{
}

lexeme::lexeme(lexeme_type type, std::string const & data, unsigned int line):
	type(type),
	data(data),
	line(line)
{
}

bool is_name_char(char input)
{
	return (is_name_char_lookup_table[input] == 1);
}

bool is_binary_digit(char input)
{
	return
		(input == '0')
			||
		(input == '1')
	;
}

unsigned int binary_string_to_number(std::string const & input)
{
	unsigned int output = 0;
	for(std::string::const_iterator i = input.begin(), end = input.end(); i != end; ++i)
	{
		char current_char = *i;
		if(is_binary_digit(current_char) == false)
		{
			break;
		}
		output = (output << 1) | (current_char - '0');
	}
	return output;
}

void parse_backslash(std::string::const_iterator & begin, std::string::const_iterator const & end, std::string & string)
{
	std::string::const_iterator offset = begin + 1;
	if(offset >= end)
	{
		throw std::runtime_error("Invalid escape sequence");
	}

	bool custom_iterator = false;

	char next_character = *offset;
	switch(next_character)
	{
		case 'a':
		{
			string += '\a';
			break;
		}

		case 'f':
		{
			string += '\f';
			break;
		}

		case 'n':
		{
			string += '\n';
			break;
		}

		case 'r':
		{
			string += '\r';
			break;
		}

		case 't':
		{
			string += '\t';
			break;
		}

		case 'v':
		{
			string += '\v';
			break;
		}

		case 'b':
		{
			string += '\b';
			break;
		}

		case '0':
		{
			unsigned int digit_counter = 8;
			for(++offset; offset < end; ++offset)
			{
				if(is_binary_digit(*offset) == false)
				{
					break;
				}
				--digit_counter;
				if(digit_counter == 0)
				{
					break;
				}
			}

			string += static_cast<char>(binary_string_to_number(std::string(begin + 1, offset)));
			begin = offset;
			custom_iterator = true;
			break;
		}

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			for(++offset; offset < end; ++offset)
			{
				if(nil::string::is_digit(*offset) == false)
				{
					break;
				}
			}

			string += static_cast<char>(binary_string_to_number(std::string(begin + 1, offset)));
			begin = offset;
			custom_iterator = true;
			break;
		}

		case 'x':
		case 'X':
		{
			unsigned int const max_hex_digits = 2;

			unsigned int digit_counter = max_hex_digits;
			for(++offset; offset < end; ++offset)
			{
				if(nil::string::is_hex_digit(*offset) == false)
				{
					break;
				}
				--digit_counter;
				if(digit_counter == 0)
				{
					break;
				}
			}

			if(digit_counter == max_hex_digits)
			{
				throw std::runtime_error("Empty hexadecimal number in escape sequence");
			}

			string += nil::string::string_to_number<char>(std::string(begin + 2, offset), std::ios_base::hex);
			begin = offset;
			custom_iterator = true;
			break;
		}

		default:
		{
			throw std::runtime_error("Unknown escape sequence in regular expression");
			break;
		}
	}

	if(custom_iterator == false)
	{
		begin += 2;
	}
}

void lexer_exception(std::string const & file_name, unsigned int line, std::string const & message)
{
	throw std::runtime_error(file_name + " (line " + nil::string::number_to_string<unsigned int>(line) + "): lexer error: " + message);
}

bool assembly_lexer(std::string const & file_name, unsigned int & line, std::string::const_iterator & begin, std::string::const_iterator const & end, lexeme & output)
{
	std::string input;

	for(; begin != end; ++begin)
	{
		char current_char = *begin;
		char type = type_lookup_table[current_char];
		switch(type)
		{
			case char_type_illegal:
			{
				lexer_exception(file_name, line, "Illegal character");
			}

			case char_type_name:
			{
				std::string::const_iterator name_begin = begin;
				for(++begin;
					(begin != end)
						&&
					(is_name_char(*begin) == true);
				++begin);
				output = lexeme(lexeme_name, std::string(name_begin, begin), line);
				return true;
			}

			case char_type_digit:
			{
				std::string::const_iterator number_begin = begin;
				for(++begin;
					(begin != end)
						&&
					(nil::string::is_digit(*begin) == true)
				; ++begin);
				output = lexeme(lexeme_number, std::string(number_begin, begin), line);
				return true;
			}

			case char_type_zero:
			{
				std::string::const_iterator number_begin = begin;
				++begin;
				if(begin != end)
				{
					char second_character = *begin;
					if(
						(second_character == 'x')
							||
						(second_character == 'X')
					)
					{
						for(++begin;
							(begin != end)
								&&
							(nil::string::is_digit(*begin) == true)
						; ++begin);
					}
					else if(is_binary_digit(second_character) == true)
					{
						for(++begin;
							(begin != end)
								&&
							(is_binary_digit(*begin) == true)
						; ++begin);
					}
				}
				output = lexeme(lexeme_number, std::string(number_begin, begin), line);
				return true;
			}

			case char_type_string:
			{
				std::string string;
				for(++begin; begin != end;)
				{
					char current_char = *begin;
					switch(current_char)
					{
						case '"':
						{
							++begin;
							output = lexeme(lexeme_string, string, line);
							return true;
						}

						case '\\':
						{
							try
							{
								parse_backslash(begin, end, string);
							}
							catch(std::exception & exception)
							{
								lexer_exception(file_name, line, exception.what());
							}
							break;
						}

						case '\n':
						{
							lexer_exception(file_name, line, "Newline in string");
						}

						default:
						{
							string += current_char;
							++begin;
							break;
						}
					}
				}

				lexer_exception(file_name, line, "Incomplete string at the end of file");
			}

			case char_type_operator:
			{
				++begin;
				output = lexeme(lexeme_operator, std::string(1, current_char), line);
				return true;
			}

			case char_type_operator_extended:
			{
				++begin;
				if(begin == end)
				{
					output = lexeme(lexeme_operator, std::string(1, current_char), line);
					return false;
				}
				else
				{
					char second_char = *begin;
					bool is_extended = false;
					switch(current_char)
					{
						case '&':
						{
							if(second_char == '&')
							{
								is_extended = true;
							}
							break;
						}

						case '|':
						{
							if(second_char == '|')
							{
								is_extended = true;
							}
							break;
						}

						case '=':
						{
							if(second_char == '=')
							{
								is_extended = true;
							}
							break;
						}

						case '!':
						{
							if(second_char == '=')
							{
								is_extended = true;
							}
							break;
						}

						case '<':
						{
							if(second_char == '=')
							{
								is_extended = true;
							}
							break;
						}

						case '>':
						{
							if(second_char == '=')
							{
								is_extended = true;
							}
							break;
						}
					}
					if(is_extended == true)
					{
						++begin;
						output = lexeme(lexeme_operator, std::string(1, current_char) + second_char, line);
					}
					else
					{
						output = lexeme(lexeme_operator, std::string(1, current_char), line);
					}
					return true;
				}
			}

			case char_type_newline:
			{
				++begin;
				++line;
				output = lexeme(lexeme_newline, line);
				return true;
			}

			case char_type_comment:
			{
				for(++begin; begin != end; ++begin)
				{
					if(*begin == '\n')
					{
						++begin;
						++line;
						output = lexeme(lexeme_newline, line);
						return true;
					}
				}
				return false;
			}
		}
	}

	return false;
}
