#include <stdexcept>
#include <fstream>
#include "utils.hpp"
#include "File.hpp"

File readFile(std::string path)
{
	std::ifstream file(path);
	if(!file.is_open())
	{
		throw std::runtime_error("Failed to open file: " + path);
	}

	std::string content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();

	if(content.size() >= 8 && content.substr(0, 8) == FileHeader::Magic)
	{

	}

	throw std::runtime_error("Failed to detect file format");
}