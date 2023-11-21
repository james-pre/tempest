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
		desc.add_options()
			("help,h", "Display help message")
			("debug,d", "Show verbose/debug messages")
			("file", "Input file");
		options::positional_options_description pos_desc;
		pos_desc.add("file", 1);
		options::variables_map vm;
		options::store(options::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
		options::notify(vm);

		if (vm.count("help"))
		{
			std::cout << "Usage: <file>" << std::endl
					  << desc << std::endl;
			return 0;
		}

		if (!vm.count("file"))
		{
			throw "No input file specified.";
		}

		std::string path = vm["file"].as<std::string>();
		std::cout << "Dump of " << path << ":\n";

		File file = readFile(path);

		std::cout << "Header: \nmagic:" << file.header.magic << "\ntype:" << std::to_string(file.header.type) << "\nversion:" << std::to_string(file.header.version) << std::endl;
		return 0;
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return 1;
	}
}
