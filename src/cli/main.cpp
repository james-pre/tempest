#include <iostream>
#include <string>
#include <fstream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
std::string project_name(PROJECT_NAME);

int main(int argc, char **argv)
{
	// Command-line options
	po::variables_map options;
	po::options_description cli("Options");
	cli.add_options()
		("help,h", "Display help message")
		("version,v", "Display version");
	po::options_description positionals;
	positionals.add_options()
		("command", "Subcommand")
		("arguments", po::value<std::vector<std::string>>(), "Arguments");
	po::positional_options_description _positionals;
	_positionals.add("command", 1);
	_positionals.add("arguments", -1);
	po::store(po::command_line_parser(argc, argv).options(po::options_description().add(cli).add(positionals)).positional(_positionals).allow_unregistered().run(), options);
	po::notify(options);

	if (options.count("version"))
	{
		std::cout << VERSION << std::endl;
		return 0;
	}

	if (options.count("help") || !options.count("command"))
	{
		std::cout << "Usage: " << project_name << " <subcommand> [options]\n" << cli << std::endl;
		return 0;
	}

	std::string subcommand = options.at("command").as<std::string>();
	std::string command = project_name + "-" + subcommand;

    for (int i = 2; i < argc; i++)
    {
        command += " " + std::string(argv[i]);
    }

	return system(command.c_str());
}
