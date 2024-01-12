#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include "../core/metadata.hpp"
#include "../core/NeuralNetwork.hpp"

namespace po = boost::program_options;

int main(int argc, char **argv)
{
	// Command-line options
	po::options_description desc("Options");
	desc.add_options()
		("help,h", "Display help message")
		("version,v", "Display version")
		("debug,d", "Show verbose/debug messages")
		("command", "Subcommand");
	po::positional_options_description pos_desc;
	pos_desc.add("command", 1);
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cout << "Usage: \n" << desc << std::endl;
		return 0;
	}

	if (vm.count("version"))
	{
		std::cout << version_name << std::endl;
		return 0;
	}

	if(!vm.count("command"))
	{
		std::cerr << "No subcommand specified." << std::endl;
		return 1;
	}

	std::string subcommand = vm.at("command").as<std::string>();

	return 0;
}
