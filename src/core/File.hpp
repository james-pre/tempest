#ifndef H_File
#define H_File

#include <string>
#include <cstdint>
#include "NeuralNetwork.hpp"
#include "Environment.hpp"

enum class FileType
{
	NONE,	 // no data
	NETWORK, // neural network(s)
	PARTIAL, // partial environment (e.g. for sharding into multiple files)
	FULL,	 // an entire environment
};

constexpr const int maxFileType = 4;

constexpr std::array<const char *, maxFileType> fileTypes = {"None", "Network", "Partial", "Full"};

typedef unsigned int FileVersion;

struct FileContents
{
	char magic[5];
	uint8_t type;
	uint16_t version;
	union Data
	{
		NeuralNetwork::Serialized *network;
	} __attribute__((packed)) data;
} __attribute__((packed));

class File
{
private:
	FileContents _contents;
public:
	FileContents &contents = _contents;
	const std::string &magic = _contents.magic;
	const FileType &type = static_cast<FileType>(_contents.type);
	const FileVersion &version = _contents.version;
	const FileContents::Data &data = _contents.data;
	File(FileContents contents);
	static File Read(std::string path);
	static constexpr char Magic[5] = "BSML";
};

#endif