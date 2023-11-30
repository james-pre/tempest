#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <boost/program_options.hpp>
#include "../core/metadata.hpp"
#include "../core/NeuralNetwork.hpp"
#include "../core/File.hpp"

namespace options = boost::program_options;

int main(int argc, char **argv)
{
	// Command-line options
	try
	{
		options::options_description desc("Options");
		desc.add_options()("help,h", "Display help message")("debug,d", "Show verbose/debug messages")("output,o", options::value<std::string>(), "Output file (for applicable output formats)")("format,f", options::value<std::string>()->default_value("text"), "Output format")("input", options::value<std::string>(), "Input file");
		options::positional_options_description pos_desc;
		pos_desc.add("input", 1);
		options::variables_map vm;
		options::store(options::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
		options::notify(vm);

		if (vm.count("help"))
		{
			std::cout << "Usage: <input>" << std::endl
					  << desc << std::endl;
			return 0;
		}

		if (!vm.count("input"))
		{
			throw std::runtime_error("No input file specified.");
		}

		const std::string path = vm["input"].as<std::string>();
		const std::string output = vm["output"].as<std::string>();
		const std::string format = vm["format"].as<std::string>();
		if (format != "text" && output.empty())
		{
			throw std::runtime_error("No output file specified.");
		}

		std::cout << "Dump of " << path << ":\n";

		const File file = File::Read(path);

		std::cout << "Header: \n\tmagic: " << file.magic << "\n";
		const unsigned char _type = file.contents.type;
		const std::string type = (_type < maxFileType) ? fileTypes.at(_type) : "Unknown (" + std::to_string(_type) + ")";
		std::cout << "\ttype: " << type << "\n\tversion: " << std::to_string(file.version) << std::endl;

		if (format == "gv" || format == "dot")
		{
		}

		if (format == "text")
		{
			switch(file.type) {
				case FileType::NONE:
					break;
				case FileType::NETWORK:
					break;
				case FileType::PARTIAL:
					break;
				case FileType::FULL:
					break;
			}
			return 0;
		}

		throw std::runtime_error("Format not supported");
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return 1;
	}
}
