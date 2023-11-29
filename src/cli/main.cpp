#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include "../core/metadata.hpp"
#include "../core/NeuralNetwork.hpp"

namespace options = boost::program_options;

int main(int argc, char **argv)
{
	// Command-line options
	options::options_description desc("Options");
	desc.add_options()
		("help,h", "Display help message")
		("version,v", "Display version")
		("debug,d", "Show verbose/debug messages")
		("output,o", options::value<std::string>()->value_name("<path>"), "The file to output to. Defaults to the same as the input.");
	options::positional_options_description pos_desc;
	pos_desc.add("file", 1);
	options::variables_map vm;
	options::store(options::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
	options::notify(vm);

	if (vm.count("help"))
	{
		std::cout << "Usage: <file>" << std::endl << desc << std::endl;
		return 0;
	}

	if (vm.count("version"))
	{
		std::cout << version_name << std::endl;
		return 0;
	}

	if(!vm.count("file"))
	{
		std::cerr << "No input file specified." << std::endl;
		return 1;
	}

	return 0;
}
