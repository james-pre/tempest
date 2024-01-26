#ifndef H_File
#define H_File

#include <string>
#include <cstring>
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

private:
	template <typename T>
	static void _writeSingle(std::ostream &output, const T &data)
	{
		output.write(reinterpret_cast<const char *>(&data), sizeof(data));
	}

	template <typename T>
	static void _readSingle(std::istream &input, T &data)
	{
		input.read(reinterpret_cast<char *>(&data), sizeof(data));
	}

protected:
	template <typename... Args>
	static void _write(std::ostream &output, const Args &...args)
	{
		(_writeSingle(output, args), ...);
	}

	template <typename... Args>
	static void _read(std::istream &input, Args &...args)
	{
		(_readSingle(input, args), ...);
	}

public:
	Header header;
	Data data;

	inline const std::string magic() const { return std::string(header.magic); }
	inline void magic(const std::string &magic) { strcpy(header.magic, magic.c_str()); }

	inline FileType type() const { return static_cast<FileType>(header.type); }
	inline void type(const FileType type) { header.type = static_cast<uint8_t>(type); }

	inline Version version() const { return header.version; }
	inline void version(const Version version) { header.version = static_cast<uint16_t>(version); }

	File()
	{
	}

	File(Header header, Data data) : header(header), data(data)
	{
	}

	void write(std::string path) const
	{
		std::ofstream output(path, std::ios::binary);
		if (!output.is_open())
		{
			throw std::runtime_error("Failed to open file: " + path);
		}

		_write(output, header);

		if (type() == FileType::NETWORK)
		{
			NeuralNetwork::Serialized net = data.network;
			_write(output, net.id, net.name, net.activation, net.neurons.size());
			for (Neuron::Serialized &neuron : net.neurons)
			{
				_write(output, neuron.id, neuron.type, neuron.outputs.size());
				for (NeuronConnection::Serialized &conn : neuron.outputs)
				{
					_write(output, conn);
				}
			}
		}
		output.close();
	}

	void read(std::string path)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input.is_open())
		{
			throw std::runtime_error("Failed to open file: " + path);
		}

		_read(input, header);

		if (type() == FileType::NETWORK)
		{
			NeuralNetwork::Serialized &net = data.network;
			size_t netSize;
			_read(input, net.id, net.name, net.activation, netSize);
			for (size_t n = 0; n < netSize; n++)
			{
				Neuron::Serialized neuron;
				size_t outputsSize;
				_read(input, neuron.id, neuron.type, outputsSize);
				for (size_t o = 0; o < outputsSize; o++)
				{
					NeuronConnection::Serialized conn;
					_read(input, conn);
					neuron.outputs.push_back(conn);
				}

				net.neurons.push_back(neuron);
			}
		}
		input.close();

		if (magic() != Magic)
		{
			throw std::runtime_error("Invalid file (bad magic)");
		}
	}

	static constexpr char Magic[5] = "TPST";
};

template <>
void File::_writeSingle(std::ostream &output, const std::string &string)
{
	_writeSingle(output, string.size());
	output.write(string.data(), string.size());
}

template <>
void File::_readSingle(std::istream &input, std::string &string)
{
	size_t size;
	_readSingle(input, size);
	string.resize(size);
	input.read(&string[0], size);
}

#endif