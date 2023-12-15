#ifndef H_File
#define H_File

#include <string>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <fstream>
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

constexpr std::array<const char *, maxFileType> fileTypes = {"none", "network", "partial", "full"};

class File
{
public:
	typedef unsigned int Version;

	union Data
	{
		NeuralNetwork::Serialized network;

		Data(const Data &other)
			: network(other.network)
		{
		}

		Data(const NeuralNetwork::Serialized &network)
			: network(network)
		{
		}

		Data() : network{} {}

		~Data() {}

		Data &operator=(const Data &other)
		{
			if (this != &other)
			{
				network = other.network;
			}
			return *this;
		}
	};

	struct Header
	{
		char magic[5];
		uint8_t type;
		uint16_t version;
	} __attribute__((packed));

	struct Contents : Header
	{
		Data data;
	};

	Contents contents;

	inline const std::string magic() const { return std::string(contents.magic); }
	inline void magic(const std::string &magic) { std::copy(magic.begin(), magic.end(), contents.magic); }

	inline FileType type() const { return static_cast<FileType>(contents.type); }
	inline void type(const FileType type) { contents.type = static_cast<uint8_t>(type); }

	inline Version version() const { return contents.version; }
	inline void version(const Version version) { contents.version = static_cast<uint16_t>(version); }

	inline const Data data() const { return contents.data; }
	inline void data(const Data &data) { contents.data = data; }

	File(Contents contents) : contents(contents)
	{
	}

	void write(std::string path) const
	{
		std::ofstream output(path, std::ios::binary);
		if (!output.is_open())
		{
			throw std::runtime_error("Failed to open file for writing: " + path);
		}

		output.write(contents.magic, sizeof(contents.magic));
		output.write(reinterpret_cast<const char *>(&contents.type), sizeof(contents.type));
		output.write(reinterpret_cast<const char *>(&contents.version), sizeof(contents.version));

		if (type() == FileType::NETWORK)
		{
			NeuralNetwork::Serialized net = data().network;
			output.write(reinterpret_cast<const char *>(&net.id), sizeof(net.id));
			size_t netSize = net.neurons.size();
			output.write(reinterpret_cast<const char *>(&netSize), sizeof(netSize));
			for (Neuron::Serialized &neuron : net.neurons)
			{
				output.write(reinterpret_cast<const char *>(&neuron.id), sizeof(neuron.id));
				output.write(reinterpret_cast<const char *>(&neuron.type), sizeof(neuron.type));
				for (NeuronConnection::Serialized &conn : neuron.outputs)
				{
					output << reinterpret_cast<const char *>(&conn);
				}
			}
		}
		output.close();
	}

	static File Read(std::string path)
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

	static constexpr char Magic[5] = "TPST";
};

#endif