#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include "../core/metadata.hpp"
#include "../core/Inspector.hpp"
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <numeric>

namespace opts = boost::program_options;

int main(int argc, char **argv)
{
	// Command-line options
	opts::options_description desc("Options");
	desc.add_options()
		("help,h", "Display help message")
		("debug,d", "Show verbose/debug messages")
		("input", opts::value<std::string>(), "Input file")
		("noload", "Do not automatically load the input file");
	opts::positional_options_description pos_desc;
	pos_desc.add("input", 1);
	opts::variables_map vm;
	opts::store(opts::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
	opts::notify(vm);

	if (vm.count("help"))
	{
		std::cout << "Usage: <input>" << std::endl
				  << desc << std::endl;
		return 0;
	}

	if (!vm.count("input"))
	{
		std::cerr << "No input file specified." << std::endl;
		return 1;
	}

	Inspector inspector;

	std::string path = vm["input"].as<std::string>();
	inspector.path(path);

	try
	{
		if (!vm.count("noload"))
		{
			inspector.load();
		}

		std::cout << "Inspecting " << path << std::endl;
		do
		{
			std::string raw_command;
			std::string scope_path = inspector.scope_path();
			std::cout << "["
					  << inspector.scope
					  << (scope_path == "" ? "" : " #")
					  << scope_path
					  << "]? ";
			std::getline(std::cin >> std::ws, raw_command);
			std::cout << inspector.exec(raw_command) << std::endl;
		} while (inspector.command()[0] != "q" && inspector.command()[0] != "quit");
		return 0;
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		if (vm.count("debug"))
		{
			throw err;
		}
		return 1;
	}
}
