#ifndef H_inspector
#define H_inspector

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <vector>
#include <string>
#include <map>
#include <numeric>
#include "utils.hpp"
#include "File.hpp"

class Inspector
{
private:

	std::string cmd_print() const
	{

		if (cmd[1] == ".")
		{
			return "";
		}

		return "Symbol can not be printed";
	}

	std::string cmd_scope_change()
	{
		if (!_loaded)
		{
			return "File not loaded";
		}

		if (file.magic() != File::Magic)
		{
			return "File invalid";
		}

		bool entering = cmd[0] == "scope:enter" || cmd[0] == ">";
		auto offset = entering ? 1 : -1;

		scope = nextTo(scopes, scope, offset);
		if (file.type() == FileType::NETWORK && scope == "environment")
		{
			scope = nextTo(scopes, scope, offset);
		}

		size_t target;
		if (entering)
		{
			if(scope == "network" && file.type() == FileType::NETWORK && cmd[1] == "\%network")
			{
				target = file.data.network.id;
			}
			else
			{
				target = std::stol(cmd[1]);
			}
			scopeValues.at(scope) = target;
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

	std::string cmd_quit() const
	{
		return "Quitting";
	}

	std::string cmd_help() const
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

	std::string cmd_list() const
	{
		if (scope == "top")
		{
			const std::string type = (file.header.type < maxFileType) ? fileTypes.at(file.header.type) : "Unknown (" + std::to_string(file.header.type) + ")";
			std::string dataID = "<error>";
			switch (file.type())
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

			return "Inspecting " + _path + ":" +
			"\nmagic: " + file.magic() +
			"\ntype: " + type +
			"\nversion: " + std::to_string(file.version()) +
			"\n\ndata: " + dataID;
		}

		if (scope == "environment")
		{
			return "Environments not supported";
		}

		if (scope == "network")
		{
			return "Inspecting network #" + std::to_string(file.data.network.id) + ":" + "\nneurons: " + std::to_string(network.size());
		}

		if (scopeValues.at("neuron") >= network.size())
		{
			return "Neuron does not exist";
		}
		Neuron::Serialized neuron = file.data.network.neurons[scopeValues.at("neuron")];
		if (scope == "neuron")
		{
			return "Inspecting neuron #" + std::to_string(neuron.id) + ":" +
			"\ntype: " + neuronTypes[neuron.type] +
			"\noutputs: " + std::to_string(neuron.outputs.size());
		}

		if (scope == "connection")
		{
			if (scopeValues.at("connection") >= neuron.outputs.size())
			{
				return "Connection does not exist";
			}
			NeuronConnection::Serialized conn = neuron.outputs[scopeValues.at("connection")];
			return "Inspecting connection to #" + std::to_string(conn.neuron) + ":" +
			"\nstrength: " + std::to_string(conn.strength) +
			"\nplasticityRate: " + std::to_string(conn.plasticityRate) +
			"\nplasticityThreshold: " + std::to_string(conn.plasticityThreshold) +
			"\nreliability: " + std::to_string(conn.reliability);
		}

		return "Invalid scope";
	}

	std::string cmd_mutate()
	{
		unsigned mutationCount;
		try{
			mutationCount = cmd.size() < 2 ? 1 : std::stoul(cmd[1]);
		} catch (const std::exception &ex) {
			mutationCount = 1;
		}
		const auto &[env, net, neuron, conn] = targets();
		if (scope == "top")
		{
			switch (file.type())
			{
			case FileType::NONE:
				return "Nothing to mutate";
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment mutation not supported";
			case FileType::NETWORK:
				for(unsigned i = 0; i < mutationCount; i++)
					net->mutate();
				_updateFileData();
				return "Mutated network #" + net->serialize().id;
			}
		}

		if(scope == "network")
		{
			for(unsigned i = 0; i < mutationCount; i++)
				net->mutate();
			_updateFileData();
			return "Mutated network";
		}

		if(scope == "neuron")
		{
			for(unsigned i = 0; i < mutationCount; i++)
				neuron->mutate();
			_updateFileData();
			return "Mutated neuron";
		}

		if(scope == "connection")
		{
			for(unsigned i = 0; i < mutationCount; i++)
				conn->mutate();
			_updateFileData();
			return "Mutated connection";
		}

		return "Invalid scope";
	}

	static const inline std::map<std::string, std::function<std::string(Inspector*)>> commands {
		{"quit", &Inspector::cmd_quit},
		{"q", &Inspector::cmd_quit},
		{"help", &Inspector::cmd_help},
		{"h", &Inspector::cmd_help},
		{"print",&Inspector::cmd_print},
		{"p", &Inspector::cmd_print},
		{"scope:enter", &Inspector::cmd_scope_change},
		{">", &Inspector::cmd_scope_change},
		{"scope:leave", &Inspector::cmd_scope_change},
		{"<", &Inspector::cmd_scope_change},
		{"list", &Inspector::cmd_list},
		{"ls", &Inspector::cmd_list},
		{"l", &Inspector::cmd_list},
		{"mutate", &Inspector::cmd_mutate},
		{"m", &Inspector::cmd_mutate},
	};

	std::tuple<Environment*, NeuralNetwork*, Neuron*, NeuronConnection*> targets()
	{
		Neuron* neuron = scopeValues.at("neuron") == SIZE_MAX ? nullptr : &(network.neuron(scopeValues.at("neuron")));
		return
		{
			nullptr,
			scopeValues.at("network") == SIZE_MAX ? nullptr : &network,
			neuron,
			scopeValues.at("connection") == SIZE_MAX && neuron != nullptr ? nullptr : &(neuron->outputs.at(scopeValues.at("connection"))),
		};
	}

	void _updateFileData()
	{
		file.data.network = network.serialize();
	}

	bool _loaded = false;
	std::string _path = "";
	std::string raw_cmd = "";
	std::vector<std::string> cmd;

	File file;
	NeuralNetwork network;

public:

	std::string scope = "top";

	std::map<std::string, size_t> scopeValues{
		{"top", 0},
		{"environment", SIZE_MAX},
		{"network", SIZE_MAX},
		{"neuron", SIZE_MAX},
		{"connection", SIZE_MAX},
	};

	bool loaded() const
	{
		return _loaded;
	}

	std::string path() const
	{
		return _path;
	}

	void path(const std::string path)
	{
		if(_loaded)
		{
			throw std::runtime_error("Can not change path while file is loaded");
		}
		_path = path;
	}

	std::vector<std::string> command() const
	{
		return cmd;
	}

	void load()
	{
		if(_loaded)
		{
			throw std::runtime_error("Already loaded");
		}

		file.read(_path);
		switch(file.type())
		{
			case FileType::NONE:
			case FileType::FULL:
			case FileType::PARTIAL:
				break;
			case FileType::NETWORK:
				network = NeuralNetwork::Deserialize(file.data.network);
		}
		_loaded = true;
	}

	void unload()
	{
		if(!_loaded)
		{
			throw std::runtime_error("Not loaded");
		}

		_loaded = false;
	}

	std::string scope_path() const
	{
		return std::accumulate(
			std::next(scopes.begin()),
			std::next(std::find(scopes.begin(), scopes.end(), scope)),
			std::string(""),
			[&](const std::string a, const std::string b)
			{
				return scopeValues.at(b) != SIZE_MAX ? a + "/" + std::to_string(scopeValues.at(b)) : a;
			});
	}

	std::string exec(std::string raw_command)
	{
		raw_cmd = raw_command;
		boost::split(cmd, raw_command, boost::is_any_of(" "));
		if(!commands.contains(cmd[0]))
		{
			return "Command does not exist";
		}
		return commands.at(cmd[0])(this);
	}

	static const inline std::vector<std::string> scopes{
		"top",
		"environment",
		"network",
		"neuron",
		"connection",
	};
};

#endif