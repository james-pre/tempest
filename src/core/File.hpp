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

struct FileHeader
{
	static constexpr char Magic[5] = "BSML";
	const char magic[5];
	uint8_t type;
	uint16_t version;
} __attribute__((packed));

union FileData
{
	NeuralNetwork::Serialized *network;
};

struct FileContents
{
	FileHeader header;
	FileData data;
} __attribute__((packed));

class File
{
private:
	FileContents contents;
public:
	const FileData &data = contents.data;
	const FileHeader &header = contents.header;
	File(FileContents contents);
	static File Read(std::string path);
};

#endif