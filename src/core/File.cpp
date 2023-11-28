#include <stdexcept>
#include <fstream>
#include "utils.hpp"
#include "File.hpp"

File::File(FileContents contents) : contents(contents)
{
}

File File::Read(std::string path)
{
	std::ifstream input(path);
	if (!input.is_open())
	{
		throw std::runtime_error("Failed to open file: " + path);
	}

	FileContents contents = {};
	input.read((char *)&contents, sizeof(contents));
	input.close();

	std::string magic = contents.header.magic;
	if (magic != FileHeader::Magic)
	{
		throw std::runtime_error("Invalid file (bad magic)");
	}

	return File(contents);
}
