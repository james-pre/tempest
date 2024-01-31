#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

namespace po = boost::program_options;

int main(int argc, char **argv)
{
	po::variables_map options;
	po::options_description cli("Options");
	cli.add_options()
		("help,h", "Display help message")
		("debug", "Show verbose/debug messages")
		("inputs,i", po::value<NeuralNetwork::values_t>()->value_name("values")->multitoken(), "Input values")
		("default", po::value<float>()->default_value(0)->value_name("value"), "Default value for missing inputs")
		("no-defaults", "Do not default missing inputs")
		("max-depth,d", po::value<unsigned>()->default_value(1000)->value_name("depth"), "The maximum depth of Neurons updates");

	po::options_description positionals("Options");
	positionals.add_options()("network", po::value<std::string>(), "Network file to run");
	po::positional_options_description _positionals;
	_positionals.add("network", 1);
	try
	{
		po::store(po::command_line_parser(argc, argv).options(po::options_description().add(cli).add(positionals)).positional(_positionals).run(), options);
	}
	catch( const std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	po::notify(options);

	if (options.count("help"))
	{
		std::cout << "Usage: <network> [options]" << std::endl
				  << cli << std::endl;
		return 0;
	}

	if (!options.count("network"))
	{
		std::cerr << "No network file specified." << std::endl;
		return 1;
	}

	const std::string path = options.at("network").as<std::string>();

	try
	{
		std::cout << "Reading " << path << "..." << std::endl;

		File file;
		file.readPath(path);
		
		if (file.type() != FileType::NETWORK)
		{
			std::cerr << "Not a network" << std::endl;
			return 1;
		}

		NeuralNetwork::values_t inputs = options.at("inputs").as<NeuralNetwork::values_t>();

		const std::vector<Neuron> inputNeurons = file.network->inputs();
		const size_t numInputNeurons = inputNeurons.size();
		
		if(inputs.size() > numInputNeurons)
		{
			std::cerr << "warning: more inputs provided than input neurons. Extra inputs discarded." << std::endl;
			inputs.resize(numInputNeurons);
		}

		const bool noDefaults = options.count("no-defaults");
		if(inputs.size() < numInputNeurons)
		{
			std::cerr << (noDefaults ? "error" : "warning") << ": less inputs provided than input neurons. ";
			if(noDefaults)
			{
				std::cerr << "No defaults allowed, exiting." << std::endl;
				return 1;
			}
			const float defaultValue = options.at("default").as<float>();
			std::cerr << " Missing inputs defaulted." << std::endl;
			inputs.resize(numInputNeurons, defaultValue);
		}

		const unsigned maxDepth = options.at("max-depth").as<unsigned>();

		std::cout << "Running..." << std::endl;

		const NeuralNetwork::values_t outputs = file.network->run(inputs, maxDepth);

		bool first = true;

		for(const float output : outputs)
		{
			if(!first)
			{
				std::cout << ",";
			}
			first = false;

			std::cout << output;
		}

		std::cout << std::endl;
		
		return 0;
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return 1;
	}
}
