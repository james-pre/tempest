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
	using Version = unsigned int;

	struct Header
	{
		char magic[5];
		uint8_t type;
		uint16_t version;
	} __attribute__((packed));

private:
	template <typename T>
	static void _write(std::ostream &output, const T &data)
	{
		output.write(reinterpret_cast<const char *>(&data), sizeof(data));
	}

	template <typename T>
	static void _read(std::istream &input, T &data)
	{
		input.read(reinterpret_cast<char *>(&data), sizeof(data));
	}

protected:
	template <typename... Args>
	static void write(std::ostream &output, const Args &...args)
	{
		(_write(output, args), ...);
	}

	template <typename... Args>
	static void read(std::istream &input, Args &...args)
	{
		(_read(input, args), ...);
	}

public:
	Header header;

	union
	{
		NeuralNetwork *network = new NeuralNetwork();
		Environment *environment;
	};

	inline const std::string magic() const { return std::string(header.magic); }
	inline void magic(const std::string &magic) { strcpy(header.magic, magic.c_str()); }

	inline FileType type() const { return static_cast<FileType>(header.type); }
	inline void type(const FileType type) { header.type = static_cast<uint8_t>(type); }

	inline Version version() const { return header.version; }
	inline void version(const Version version) { header.version = static_cast<uint16_t>(version); }

	File() {}

	File(Header header) : header(header) {}

	~File() {}

	void writePath(const std::string &path) const
	{
		std::ofstream output(path, std::ios::binary);
		if (!output.is_open())
		{
			throw std::runtime_error("Failed to open file: " + path);
		}

		write(output, header);
		switch (type())
		{
		case FileType::NONE:
			break;
		case FileType::PARTIAL:
			break;
		case FileType::FULL:
			write(output, *environment);
			break;
		case FileType::NETWORK:
			write(output, *network);
			break;
		}
		output.close();
	}

	void readPath(const std::string &path /*, Data &data*/)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input.is_open())
		{
			throw std::runtime_error("Failed to open file: " + path);
		}

		read(input, header);
		switch (type())
		{
		case FileType::NONE:
			break;
		case FileType::PARTIAL:
			break;
		case FileType::FULL:
			read(input, *environment);
			break;
		case FileType::NETWORK:
			read(input, *network);
			break;
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
void File::_write(std::ostream &output, const std::string &string)
{
	_write(output, string.size());
	output.write(string.data(), string.size());
}

template <>
void File::_write(std::ostream &output, const Neuron::Connection &conn)
{
	_write(output, static_cast<Neuron::ConnectionData>(conn));
}

template <>
void File::_write(std::ostream &output, const NeuralNetwork &net)
{
	write(output, net.id, net.name, net.activation, net.size());

	for (const auto &[id, neuron] : net)
	{
		write(output, id, static_cast<uint8_t>(neuron.type), neuron.outputs.size());
		for (const Neuron::Connection &conn : neuron.outputs)
		{
			write(output, conn);
		}
	}
}

template <>
void File::_read(std::istream &input, std::string &string)
{
	size_t size;
	_read(input, size);
	string.resize(size);
	input.read(&string[0], size);
}

template <>
void File::_read(std::istream &input, Neuron::Connection &conn)
{
	_read<Neuron::ConnectionData>(input, conn);
}

template <>
void File::_read(std::istream &input, NeuralNetwork &net)
{
	size_t netSize;
	read(input, net.id, net.name, net.activation, netSize);
	for (size_t n = 0; n < netSize; n++)
	{
		Neuron neuron;
		size_t outputsSize;
		uint8_t type;
		read(input, neuron._id, type, outputsSize);
		neuron.type = static_cast<NeuronType>(type);
		for (size_t o = 0; o < outputsSize; o++)
		{
			Neuron::Connection conn;
			read(input, conn);
			neuron.outputs.push_back(conn);
		}

		net.add(neuron);
	}
}

#endif