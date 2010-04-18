#include "parser.hpp"

#include <map>

#include <nil/file.hpp>
#include <nil/string.hpp>

#include "lexer.hpp"

#include <iostream>

bool read_until_eol(std::string const & file_name, unsigned int & line, std::string::const_iterator & begin, std::string::const_iterator const & end, std::vector<lexeme> & output)
{
	while(true)
	{
		lexeme current_lexeme;
		bool not_done_yet = assembly_lexer(file_name, line, begin, end, current_lexeme);
		if(not_done_yet == true)
		{
			if(current_lexeme.type == lexeme_newline)
			{
				return true;
			}
			output.push_back(current_lexeme);
		}
		else
		{
			break;
		}
	}
	return false;
}

bool include_file(std::string const & file_name, std::string const & path, std::vector<std::string> const & include_directories, std::string & output)
{
	bool success = nil::read_file(file_name, output);
	if(success == true)
	{
		return true;
	}
	success = nil::read_file(path + file_name, output);
	if(success == true)
	{
		return true;
	}
	for(std::vector<std::string>::const_iterator i = include_directories.begin(), end = include_directories.end(); i != end; ++i)
	{
		success = nil::read_file(*i + file_name, output);
		if(success == true)
		{
			return true;
		}
	}
	return false;
}

void parser_exception(std::string const & file_name, unsigned int line, std::string const & message)
{
	throw std::runtime_error(file_name + " (line " + nil::string::number_to_string<unsigned int>(line) + "): parser error: " + message);
}

void actual_assembly_parser(std::string const & file_name, std::string const & path, std::vector<std::string> const & include_directories)
{
	std::string input;
	bool success = nil::read_file(file_name, input);
	if(success == false)
	{
		throw std::runtime_error("Failed to open file \"" + file_name + "\"");
	}

	std::string::iterator
		begin = input.begin(),
		end = input.end();

	typedef std::map< std::string, std::vector<lexeme> > macro_map;

	macro_map macros;
	std::vector<lexeme> lexemes;

	unsigned int line = 1;
	while(true)
	{
		lexeme current_lexeme;
		bool not_done_yet = assembly_lexer(file_name, line, begin, end, current_lexeme);
		if(not_done_yet == true)
		{
			if(current_lexeme.type == lexeme_name)
			{
				std::vector<lexeme> line_lexemes;

				std::string const & name = current_lexeme.data;
				if(name == "include")
				{
					read_until_eol(file_name, line, begin, end, line_lexemes);
					if(line_lexemes.size() != 1)
					{
						parser_exception(file_name, line, "invalid argument count in include statement");
					}
					else if(line_lexemes[0].type != lexeme_string)
					{
						parser_exception(file_name, line, "invalid argument type in include statement");
					}
					std::string include_input;
					std::string const & include_file_name = line_lexemes[0].data;
					bool success = include_file(include_file_name, path, include_directories, include_input);
					if(success == false)
					{
						parser_exception(file_name, line, "failed to include file \"" + include_file_name + "\"");
					}
					input = include_input + std::string(begin, end);
					begin = input.begin();
					end = input.end();
				}
				else if(name == "define")
				{
					bool not_done_yet = read_until_eol(file_name, line, begin, end, line_lexemes);
					if(not_done_yet == false)
					{
						return;
					}
					else if(line_lexemes.size() < 1)
					{
						parser_exception(file_name, line, "invalid argument count in define statement");
					}
					else if(line_lexemes[0].type != lexeme_name)
					{
						parser_exception(file_name, line, "expected a name in define statement");
					}
					std::string define_name = line_lexemes[0].data;
					line_lexemes.erase(line_lexemes.begin());
					for(std::vector<lexeme>::iterator i = line_lexemes.begin(), end = line_lexemes.end(); i != end; ++i)
					{
						if(i->type == lexeme_name)
						{
							macro_map::iterator
								macros_end = macros.end(),
								search = macros.find(i->data);
							if(search != macros_end)
							{
								std::size_t backup = i - line_lexemes.begin();
								std::vector<lexeme> & macro_lexemes = search->second;
								line_lexemes.insert(i + 1, macro_lexemes.begin(), macro_lexemes.end());
								line_lexemes.erase(line_lexemes.begin() + backup);
								i = line_lexemes.begin() + backup + macro_lexemes.size();
								end = line_lexemes.end();
							}
						}
					}
					macros[define_name] = line_lexemes;
				}
				else
				{
					macro_map::iterator
						macros_end = macros.end(),
						search = macros.find(name);
					if(search == macros_end)
					{
						lexemes.push_back(current_lexeme);
					}
					else
					{
						std::vector<lexeme> & macro_lexemes = search->second;
						lexemes.insert(lexemes.end(), macro_lexemes.begin(), macro_lexemes.end());
					}
				}
			}
			else
			{
				lexemes.push_back(current_lexeme);
			}
		}
		else
		{
			break;
		}
	}

	unsigned int
		code_boundary = 0,
		data_boundary = 0;

	for(std::vector<lexeme>::iterator i = lexemes.begin(), end = lexemes.end(); i != end; ++i)
	{
		switch(i->type)
		{
			case lexeme_name:
			{
				std::vector<lexeme>::iterator line_begin = i;
				for(
					std::vector<lexeme>::iterator i = lexemes.begin(), end = lexemes.end();
					(i != end)
						&&
					(i->type != lexeme_newline);
					++i
				);
				std::size_t line_length = i - line_begin;
				std::string const & name = line_begin->data;
				if(name == "align")
				{
					if(line_length != 3)
					{
						parser_exception(file_name, line_length->line, "Illegal argument count for align statement");
					}
				}
				++i;
				break;
			}

			case lexeme_number:
			case lexeme_string:
			case lexeme_operator:
			{
				parser_exception(file_name, i->line, "Illegal lexeme type at the beginning of a line");
			}
		}
	}
}

void assembly_parser(std::string file_name, std::vector<std::string> include_directories)
{
	for(std::vector<std::string>::iterator i = include_directories.begin(), end = include_directories.end(); i != end; ++i)
	{
		std::string & current_string = *i;
		if(current_string.empty() == true)
		{
			throw std::runtime_error("Empty include directory string");
		}
		char last_char = *(current_string.end() - 1);
		if(last_char != '\\')
		{
			current_string += '\\';
		}
	}

	std::string path;
	std::size_t last_backslash = file_name.rfind('\\');
	if(last_backslash != std::string::npos)
	{
		++last_backslash;
		path = file_name.substr(0, last_backslash);
	}
	actual_assembly_parser(file_name, path, include_directories);
}
