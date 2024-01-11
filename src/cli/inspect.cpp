#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include "../core/metadata.hpp"
#include "../core/utils.hpp"
#include <boost/program_options.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <numeric>

std::string path;
std::string raw_command;
std::vector<std::string> command;
bool file_loaded = false;

File file;
NeuralNetwork::Serialized network;

std::map<std::string, size_t> scopeValues{
	{"top", 0},
	{"environment", SIZE_MAX},
	{"network", SIZE_MAX},
	{"neuron", SIZE_MAX},
	{"connection", SIZE_MAX},
};

std::vector<std::string> scopes{
	"top",
	"environment",
	"network",
	"neuron",
	"connection",
};

std::string scope = "top";

std::string cmd_print()
{

	if (command[1] == ".")
	{
		return "";
	}

	return "Symbol can not be printed";
}

std::string cmd_scope_change()
{
	if (!file_loaded)
	{
		return "File not loaded";
	}

	if (file.magic() != File::Magic)
	{
		return "File invalid";
	}

	bool entering = command[0] == "scope:enter" || command[0] == ">";
	auto offset = entering ? 1 : -1;

	scope = nextTo(scopes, scope, offset);
	if (file.type() == FileType::NETWORK && scope == "environment")
	{
		scope = nextTo(scopes, scope, offset);
	}

	size_t target;
	if (entering)
	{
		target = std::stol(command[1]);
		scopeValues[scope] = target;
	}
	switch (file.type())
	{
	case FileType::NONE:
		return "No file data";
	case FileType::PARTIAL:
	case FileType::FULL:
		return "Environments not supported";
	case FileType::NETWORK:
		return "Changed scope to \"" + scope + "\"" + (entering ? " (#" + std::to_string(target) + ")" : "");
	}

	return "Failed to change scope";
}

std::string cmd_quit()
{
	return "Quitting";
}

std::string cmd_help()
{
	return R"(Inspector commands:
	help, h				Display this help message
	quit, q				Quit the inspector
	print,p <symbol>	Print <symbol>
	list,l				List info about the current scope target
	scope:enter <id>	
	scope:leave			
	)";
}

std::string cmd_list()
{
	if(scope == "top")
	{
		const std::string type = (file.header.type < maxFileType) ? fileTypes.at(file.header.type) : "Unknown (" + std::to_string(file.header.type) + ")";
		std::string dataID = "<error>";
		switch(file.type())
		{
			case FileType::NONE:
				dataID = "<none>";
				break;
			case FileType::PARTIAL:
			case FileType::FULL:
				dataID = "<not supported>";
				break;
			case FileType::NETWORK:
				dataID = "#" + std::to_string(file.data.network.id);
				break;
		}
		
		return "Inspecting " + path + ":"
		+ "\nmagic: " + file.magic()
		+ "\ntype: " + type
		+ "\nversion: " + std::to_string(file.version())
		+ "\n\ndata: " + dataID;
	}

	if(scope == "environment")
	{
		return "Environments not supported";
	}

	if(scope == "network")
	{
		return "Inspecting network #" + std::to_string(network.id) + ":"
		+ "\nneurons: " + std::to_string(network.neurons.size());
	}

	if(scopeValues["neuron"] >= network.neurons.size())
	{
		return "Neuron does not exist";
	}
	Neuron::Serialized neuron = network.neurons[scopeValues["neuron"]];
	if(scope == "neuron")
	{
		return "Inspecting neuron #" + std::to_string(neuron.id) + ":"
		+ "\ntype: " + neuronTypes[neuron.type]
		+ "\noutputs: " + std::to_string(neuron.outputs.size());

	}

	if(scope == "connection")
	{
		if(scopeValues["connection"] >= neuron.outputs.size())
		{
			return "Connection does not exist";
		}
		NeuronConnection::Serialized conn = neuron.outputs[scopeValues["connection"]];
		return "Inspecting connection to #" + std::to_string(conn.neuron) + ":"
		+ "\nstrength: " + std::to_string(conn.strength)
		+ "\nplasticityRate: " + std::to_string(conn.plasticityRate)
		+ "\nplasticityThreshold: " + std::to_string(conn.plasticityThreshold)
		+ "\nreliability: " + std::to_string(conn.reliability);
	}

	return "Invalid scope";
}

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

	path = vm["input"].as<std::string>();

	std::map<std::string, std::function<std::string()>> handlers = {
		{"quit", cmd_quit},
		{"q", cmd_quit},
		{"help", cmd_help},
		{"h", cmd_help},
		{"print", cmd_print},
		{"p", cmd_print},
		{"scope:enter", cmd_scope_change},
		{">", cmd_scope_change},
		{"scope:leave", cmd_scope_change},
		{"<", cmd_scope_change},
		{"list", cmd_list},
		{"l", cmd_list},
	};

	try
	{
		if (!vm.count("noload"))
		{
			file.read(path);
			network = file.data.network;
			file_loaded = true;
		}

		std::cout << "Inspecting " << path << std::endl;
		do
		{
			
			std::string scope_path = std::accumulate(
				std::next(scopes.begin()),
				std::next(std::find(scopes.begin(), scopes.end(), scope)),
				std::string(""),
				[](const std::string a, const std::string b)
				{
					return scopeValues[b] != SIZE_MAX ? a + "/" + std::to_string(scopeValues[b]) : a;
				});
			std::cout << "["
			<< scope
			<< (scope_path == "" ? "" : " #")
			<< scope_path 
			<< "]? ";
			std::getline(std::cin >> std::ws, raw_command);
			boost::split(command, raw_command, boost::is_any_of(" "));
			std::cout << (handlers.contains(command[0]) ? handlers[command[0]]() : "Command does not exist") << std::endl;
		} 
		while (command[0] != "q" && command[0] != "quit");
		return 0;
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		if(vm.count("debug"))
		{
			throw err;
		}
		return 1;
	}
}
