#ifndef H_File
#define H_File

#include <string>
#include <cstdint>
#include <algorithm>
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

class File
{
public:
	typedef unsigned int Version;

	union Data
	{
		NeuralNetwork::Serialized *network;
	} __attribute__((packed));

	struct Contents
	{
		char magic[5];
		uint8_t type;
		uint16_t version;
		Data data;
	} __attribute__((packed));

	Contents contents;

	inline const std::string magic() const { return std::string(contents.magic); }
	inline void magic(const std::string &magic) { std::copy(magic.begin(), magic.end(), contents.magic); }

	inline FileType type() const { return static_cast<FileType>(contents.type); }
	inline void type(const FileType type) { contents.type = static_cast<uint8_t>(type); }

	inline Version version() const { return contents.version; }
	inline void version(const Version version) { contents.version = static_cast<uint16_t>(version); }

	inline const Data data() const { return contents.data; }
	inline void data(const Data &data) { contents.data = data; }

	File(Contents contents);

	static File Read(std::string path);

	static constexpr char Magic[5] = "BSML";
};

#endif