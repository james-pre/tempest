#include <stdexcept>
#include <fstream>
#include "utils.hpp"
#include "File.hpp"

File::File(FileContents contents) : _contents(contents),
									contents(_contents),
									magic(contents.magic),
									type(static_cast<FileType>(contents.type)),
									version(_contents.version),
									data(_contents.data)
{
}

File File::Read(std::string path)
{
	std::ifstream input(path);
	if (!input.is_open())
	{
		throw std::runtime_error("Failed to open file: " + path);
	}

	const FileContents contents = {};
	input.read((char *)&contents, sizeof(contents));
	input.close();

	std::string magic = contents.magic;
	if (magic != File::Magic)
	{
		throw std::runtime_error("Invalid file (bad magic)");
	}

	return File(contents);
}
