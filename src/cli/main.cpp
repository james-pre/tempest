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
		std::cout << VERSION_NAME << std::endl;
		return 0;
	}

	if(!vm.count("file"))
	{
		std::cerr << "No input file specified." << std::endl;
		return 1;
	}

	/*
	// Provide input values
	std::vector<float> inputValues = {2, 3};

	// Activate the neural network with input values and the activation function
	std::vector<float> outputValues = neuralNetwork->processInput(inputValues);

	// Print output values
	std::cout << "Output values:";
	for (float outputValue : outputValues)
	{
		std::cout << " " << outputValue;
	}
	std::cout << std::endl;

	if (vm.count("output"))
	{
		std::string outputPath = vm["output"].as<std::string>();
		std::cout << "Writing output to " << outputPath << std::endl;
		std::ofstream file(outputPath);

		if(format == "json")
		{
			file << neuralNetwork->to_json() << std::endl;
		}
		else
		{
			std::cerr << "output format \"" << format << "\" not supported" << std::endl;
		}
		file.close();
	}

	// Clean up
	delete neuralNetwork;*/

	return 0;
}
