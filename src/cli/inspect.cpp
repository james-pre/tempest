#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include "../core/Inspector.hpp"
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <numeric>

namespace po = boost::program_options;

int main(int argc, char **argv)
{
	// Command-line options
	po::options_description cli("Options");
	cli.add_options()
		("help,h", "Display help message")
		("debug", "Show verbose/debug messages")
		("no-load", "Do not automatically load the input file");
	po::options_description _positionals;
	_positionals.add_options()("input", po::value<std::string>()->value_name("path"), "Input file");
	po::positional_options_description positionals;
	positionals.add("input", 1);
	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).options(po::options_description().add(cli).add(_positionals)).positional(positionals).run(), options);
	po::notify(options);

	if (options.count("help"))
	{
		std::cout << "Usage: [input] [options]" << std::endl
				  << cli << std::endl;
		return 0;
	}

	Inspector inspector;

	std::string path;
	
	if (options.count("input"))
	{
		path = options.at("input").as<std::string>();
	}
	else
	{
		std::cout << "File: ";
		std::getline(std::cin >> std::ws, path);
	}
	
	inspector.path(path);

	try
	{
		if (!options.count("no-load"))
		{
			inspector.load();
		}

		std::cout << "Inspecting " << path << std::endl;
		do
		{
			std::string raw_command;
			std::string scope_path = inspector.stringify_scope();
			std::cout << "["
					  << inspector.scope.active
					  << (scope_path == "" ? "" : " #")
					  << scope_path
					  << "]? ";
			std::getline(std::cin >> std::ws, raw_command);
			std::cout << inspector.exec(raw_command) << std::endl;
		}
		while (inspector.command() != "quit");
		return 0;
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		if (options.count("debug"))
		{
			throw err;
		}
		return 1;
	}
}
