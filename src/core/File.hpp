#ifndef H_File
#define H_File

#include <string>
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
	static constexpr char Magic[6] = "ZENML";
	const char magic[6];
	unsigned char type;
	unsigned short int version;
};

struct File
{
	FileHeader header;
};

File readFile(std::string path);
//FileMetadata readMetadata(std::string path);
NeuralNetwork readNetwork(std::string path);
Environment readEnvironment(std::string path);

#endif