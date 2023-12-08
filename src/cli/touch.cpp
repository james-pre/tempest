#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include "../core/metadata.hpp"
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace opts = boost::program_options;

int main(int argc, char **argv)
{
	// Command-line options
	opts::options_description desc("Options");
	desc.add_options()
		("help,h", "Display help message")
		("debug,d", "Show verbose/debug messages")
		("output", opts::value<std::string>(), "Output file")
		("force,f", "Overwrite files if they already exist")
		("type,t", opts::value<std::string>()->default_value("none"),"type")
		("version,v", opts::value<uint16_t>()->default_value(0));
	opts::positional_options_description pos_desc;
	pos_desc.add("output", 1);
	opts::variables_map vm;
	opts::store(opts::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
	opts::notify(vm);

	if (vm.count("help"))
	{
		std::cout << "Usage: <input>" << std::endl
				  << desc << std::endl;
		return 0;
	}

	const std::string path = vm["output"].as<std::string>();
	if (path.empty())
	{
		std::cerr << "No output file specified." << std::endl;
		return 1;
	}

	std::ofstream output(path);
	if (output.bad() && !vm.count("force"))
	{
		std::cerr << "Ouput file exists (use force to overwrite)" << std::endl;
		return 1;
	}

	std::cout << "Creating " << path << "\n";

	File file({});

	file.magic(File::Magic);

	FileType type;
	std::string type_string = vm["type"].as<std::string>();
	std::transform(type_string.begin(), type_string.end(), type_string.begin(), ::tolower);
	auto type_it = std::find(fileTypes.begin(), fileTypes.end(), type_string);
	if (type_it != fileTypes.end())
	{
		type = static_cast<FileType>(std::distance(fileTypes.begin(), type_it));
	}
	else
	{
		size_t pos;
		try
		{
			unsigned long type_value = std::stoul(type_string, &pos);
			if (pos != type_string.length())
			{
				std::cout << "Warning: type truncated" << std::endl;
			}

			if (type_value > std::numeric_limits<uint8_t>::max())
			{
				std::cerr << "type to large" << std::endl;
				return 1;
			}

			type = static_cast<FileType>(type_value);
		}
		catch (const std::exception &ex)
		{
			std::cerr << ex.what() << std::endl;
		}
	}
	file.type(type);

	File::Version version;
	const std::string version_string = vm["version"].as<std::string>();
	try
	{
		size_t pos;
		unsigned long value = std::stoul(version_string, &pos);
		if (pos != version_string.length())
		{
			std::cout << "Warning: type truncated" << std::endl;
		}

		if (value > std::numeric_limits<uint8_t>::max())
		{
			std::cerr << "type to large" << std::endl;
			return 1;
		}

		version = static_cast<File::Version>(value);
	}
	catch (const std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
	}
	file.version(version);



	file.type();

	output.flush();
	output.close();
	return 0;
}
