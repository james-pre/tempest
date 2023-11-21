#include <stdexcept>
#include <fstream>
#include "utils.hpp"
#include "File.hpp"

File readFile(std::string path)
{
	std::ifstream input(path);
	if(!input.is_open())
	{
		throw std::runtime_error("Failed to open file: " + path);
	}

	File file = {};
	input.read((char*) &file, sizeof(file));
    input.close();

	std::string magic = file.header.magic;
	if(magic != FileHeader::Magic)
	{
		throw std::runtime_error("Invalid file (magic: " + magic + ")");
	}

	return file;
}