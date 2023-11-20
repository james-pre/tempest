#ifndef H_File
#define H_File

#include <string>
#include <cstdint>
#include "NeuralNetwork.hpp"
#include "Environment.hpp"

enum class FileType
{
	NONE,	 // no data
	NETWORK, // just a network
	PARTIAL, // partial environment (e.g. for sharding into multiple files)
	FULL,	 // an entire environment
};

struct FileHeader
{
	static constexpr char Magic[4] = { 'B', 'S', 'M', 'L' };
	const char magic[4];
	uint8_t type;
	uint16_t version;
} __attribute__((packed));

struct File
{
	FileHeader header;
} __attribute__((packed));

File readFile(std::string path);
//FileMetadata readMetadata(std::string path);
NeuralNetwork readNetwork(std::string path);
Environment readEnvironment(std::string path);

#endif