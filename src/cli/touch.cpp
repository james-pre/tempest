#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace po = boost::program_options;

int main(int argc, char **argv)
{

	po::variables_map options;

	po::options_description cli("Options");
	cli.add_options()("help,h", "Display help message")("debug,d", "Show verbose/debug messages")("force,f", "Overwrite files if they already exist")("config,c", po::value<std::string>(), "Configuration file");

	po::options_description config("Configuration");
	config.add_options()("type,t", po::value<std::string>()->default_value("none"), "file type")("version,v", po::value<unsigned int>()->default_value(0), "file version")("neurons", po::value<unsigned int>()->default_value(0), "number of neurons to create per network")("mutations", po::value<unsigned int>()->default_value(0), "number of mutations");
	//("mutations-per-neuron", po::value<bool>()->default_value(true), "whether the number of mutations is per neuron (true) or per network (false)");

	po::options_description _positionals;
	_positionals.add_options()("output", po::value<std::string>());
	po::positional_options_description positionals;
	positionals.add("output", 1);

	po::store(po::command_line_parser(argc, argv).options(po::options_description().add(cli).add(config).add(_positionals)).positional(positionals).run(), options);
	if (options.count("config"))
	{
		std::string config_path = options.at("config").as<std::string>();
		std::ifstream config_file(config_path);
		if (!config_file.is_open())
		{
			std::cout << "Failed to open configuration file \"" << config_path << "\", skipping.";
		}
		else
		{
			po::store(po::parse_config_file(config_file, config), options);
		}
	}
	po::notify(options);

	if (options.count("help"))
	{
		std::cout << "Usage: <output>" << std::endl
				  << cli << std::endl;
		return 0;
	}

	if (!options.count("output"))
	{
		std::cerr << "No output file specified." << std::endl;
		return 1;
	}
	const std::string path = options.at("output").as<std::string>();
	if (path.empty())
	{
		std::cerr << "No output file specified." << std::endl;
		return 1;
	}

	std::cout << "Creating " << path << "\n";

	File file({});

	file.magic(File::Magic);

	FileType type;
	std::string type_string = options.at("type").as<std::string>();
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

	File::Version version = options.at("version").as<File::Version>();
	file.version(version);

	if (type == FileType::NETWORK)
	{
		NeuralNetwork network(0);
		unsigned num_neurons = options.at("neurons").as<unsigned>();
		unsigned num_mutations = options.at("mutations").as<unsigned>();
		for (unsigned i = 0; i < num_neurons; i++)
		{
			network.createNeuron(static_cast<NeuronType>(i % 4));
		}

		for (auto &[id, neuron] : network._neurons)
		{
			for (unsigned i = 0; i < num_mutations; i++)
			{
				neuron.mutate();
			}
		}
		file.data = File::Data(network.serialize());
	}

	file.write(path);
	return 0;
}
