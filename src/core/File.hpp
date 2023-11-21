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
	static constexpr char Magic[5] = "BSML";
	const char magic[5];
	uint8_t type;
	uint16_t version;
} __attribute__((packed));

struct File
{
	FileHeader header;
} __attribute__((packed));

File readFile(std::string path);

#endif