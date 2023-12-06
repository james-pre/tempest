#include <stdexcept>
#include <fstream>
#include "utils.hpp"
#include "File.hpp"

File::File(Contents contents) : contents(contents)
{
}

File File::Read(std::string path)
{
	std::ifstream input(path);
	if (!input.is_open())
	{
		throw std::runtime_error("Failed to open file: " + path);
	}

	const Contents contents = {};
	input.read((char *)&contents, sizeof(contents));
	input.close();

	std::string magic = contents.magic;
	if (magic != Magic)
	{
		throw std::runtime_error("Invalid file (bad magic)");
	}

	return File(contents);
}