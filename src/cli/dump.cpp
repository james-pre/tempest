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
		("output,o", opts::value<std::string>()->default_value("/proc/stdout"), "Output file (for applicable output formats)")
		("format,f", opts::value<std::string>()->default_value("text"),"Output format")
		("input", opts::value<std::string>(), "Input file");
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

	const std::string path = vm["input"].as<std::string>();
	const std::string output = vm["output"].as<std::string>();
	const std::string format = vm["format"].as<std::string>();
	if (format != "text" && (output.empty() || output == "/proc/stdout"))
	{
		std::cerr << "No output file specified." << std::endl;
		return 1;
	}
	try
	{
		std::cout << "Dump of " << path << ":\n";

		const File file = File::Read(path);

		std::cout << "Header: \n\tmagic: " << file.magic() << "\n";
		const unsigned char _type = file.contents.type;
		const std::string type = (_type < maxFileType) ? fileTypes.at(_type) : "Unknown (" + std::to_string(_type) + ")";
		std::cout << "\ttype: " << type << "\n\tversion: " << std::to_string(file.version()) << std::endl;

		if (file.type() == FileType::NONE)
		{
			std::cout << "No data" << std::endl;
			return 0;
		}

		std::ofstream out(output);
		File::Data data = file.data();

		if (format == "gv" || format == "dot")
		{

			switch (file.type())
			{
			case FileType::NONE:
				out << "";
				break;
			case FileType::NETWORK:
				out << "digraph net_" << std::to_string(data.network.id) << " {\n";
				for(size_t i = 0; i < data.network.num_neurons; i++) {
					Neuron::Serialized &neuron = data.network.neurons[i];
					out << "\tn_" << std::to_string(neuron.id) << " -> {";
					for(size_t o = 0; o < neuron.num_outputs; o++) {
						NeuronConnection::Serialized &conn = neuron.outputs[i];
						if(o != 0) {
							out << ',';
						}
						out << std::to_string(conn.neuron);
					}
					out << "}\n";
				}
				break;
			case FileType::PARTIAL:
				out << "Not supported" << std::endl;
				break;
			case FileType::FULL:
				out << "Not supported" << std::endl;
				break;
			}
		}

		else if (format == "text")
		{		

			if (file.type() == FileType::NONE) {
				out << "";
			}
			if (file.type() == FileType::NETWORK) {
				out << "Network " << std::to_string(data.network.id) << ":\n";
				/*Neuron::Serialized* _neurons = new Neuron::Serialized[data.network.num_neurons];
				if(_neurons == nullptr) {
					std::cerr << "Allocation for neurons failed." << std::endl;
					return 1;
				}
				std::memcpy(_neurons, data.network.neurons, sizeof(Neuron::Serialized) * data.network.num_neurons);*/
				for(size_t i = 0; i < data.network.num_neurons; i++) {
					Neuron::Serialized neuron = data.network.neurons[i];
					out << "\tNeuron " << std::to_string(neuron.id) << "(" << neuron.type << "," << neuron.num_outputs << " outputs):\n";
					for(size_t o = 0; o < neuron.num_outputs; o++) {
						NeuronConnection::Serialized &conn = neuron.outputs[o];
						out << "" << std::to_string(conn.neuron) << " (" << std::to_string(conn.strength) << ", " << std::to_string(conn.plasticityRate) << ", " << std::to_string(conn.plasticityThreshold) << ", " << std::to_string(conn.reliability) << ")";
					}
					out << '\n';
				}
				//delete[] _neurons;
			}
			if (file.type() == FileType::PARTIAL) {
				out << "Not supported" << std::endl;
			}
			if (file.type() == FileType::FULL) {
				out << "Not supported" << std::endl;
			}
		}

		else
		{
			std::cerr << "Format not supported" << std::endl;
		}

		out.flush();
		out.close();
		return 0;
		
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return 1;
	}
}
