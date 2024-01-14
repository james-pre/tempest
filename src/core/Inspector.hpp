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
public:
	class Scope
	{
	public:
		std::string active = "top";

		std::map<std::string, size_t> values{
			{"top", 0},
			{"environment", SIZE_MAX},
			{"network", SIZE_MAX},
			{"neuron", SIZE_MAX},
			{"connection", SIZE_MAX},
		};

		void restore(Scope other)
		{
			active = other.active;
			for(const std::string &scope : scopes)
			{
				values[scope] = other.values[scope];
			}
		}

		void next(ptrdiff_t offset = 1, std::vector<std::string> skip = {})
		{
			active = nextTo(scopes, active, offset);
			while(std::find(skip.begin(), skip.end(), active) != skip.end() && active != scopes.front() && active != scopes.back())
			{
				active = nextTo(scopes, active, offset);
			}
		}

		std::string path() const
		{
			return std::accumulate(
				std::next(scopes.begin()),
				std::next(std::find(scopes.begin(), scopes.end(), active)),
				std::string(""),
				[&](const std::string a, const std::string b)
				{
					return values.at(b) != SIZE_MAX ? a + "/" + std::to_string(values.at(b)) : a;
				});
		}
	};

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

		scope.next(offset, { file.type() ==  FileType::NETWORK ? "environment" : "" });

		size_t target;
		if (entering)
		{
			if (scope.active == "network" && file.type() == FileType::NETWORK && cmd[1] == "\%")
			{
				target = file.data.network.id;
			}
			else
			{
				target = std::stoul(cmd[1]);
			}
			scope.values.at(scope.active) = target;
		}
		switch (file.type())
		{
		case FileType::NONE:
			return "No file data";
		case FileType::PARTIAL:
		case FileType::FULL:
			return "Environments not supported";
		case FileType::NETWORK:
			return "Changed scope to \"" + scope.active + "\"" + (entering ? " (#" + std::to_string(target) + ")" : "");
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
	help,h				Display this help message
	quit,q				Quit the inspector
	info,i [symbol]		List info about the current scope target
	scope:enter,> <scope>	
	scope:leave,<		
	)";
	}

	std::string cmd_info()
	{
		Scope scope_copy(scope);
		bool scope_changed = false;

		for (size_t i = 1; i < cmd.size(); i++)
		{
			if (i >= scopes.size())
			{
				break;
			}

			scope.next(1, { file.type() ==  FileType::NETWORK ? "environment" : "" });
			scope.values[scope.active] = std::stoul(cmd[i]);
			scope_changed = true;
		}
		if (scope.active == "top")
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

			if (scope_changed) scope.restore(scope_copy);
			return "Inspecting " + _path + ":" +
				   "\nmagic: " + file.magic() +
				   "\ntype: " + type +
				   "\nversion: " + std::to_string(file.version()) +
				   "\n\ndata: " + dataID;
		}

		if (scope.active == "environment")
		{
			if (scope_changed) scope.restore(scope_copy);
			return "Environments not supported";
		}

		if (scope.active == "network")
		{
			if (scope_changed) scope.restore(scope_copy);
			return "Inspecting network #" + std::to_string(file.data.network.id) + ":" +
				   "\nneurons: " + std::to_string(network.size());
		}

		if (scope.values.at("neuron") >= network.size())
		{
			if (scope_changed) scope.restore(scope_copy);
			return "Neuron does not exist";
		}
		Neuron::Serialized neuron = file.data.network.neurons[scope.values.at("neuron")];
		if (scope.active == "neuron")
		{
			if (scope_changed) scope.restore(scope_copy);
			return "Inspecting neuron #" + std::to_string(neuron.id) + ":" +
				   "\ntype: " + neuronTypes[neuron.type] +
				   "\noutputs: " + std::to_string(neuron.outputs.size());
		}

		if (scope.active == "connection")
		{
			if (scope.values.at("connection") >= neuron.outputs.size())
			{
				if (scope_changed) scope.restore(scope_copy);
				return "Connection does not exist";
			}
			NeuronConnection::Serialized conn = neuron.outputs[scope.values.at("connection")];
			if (scope_changed) scope.restore(scope_copy);
			return "Inspecting connection #" + std::to_string(scope.values.at("connection")) + ":" +
				   "\nto: #" + std::to_string(conn.neuron) +
				   "\nstrength: " + std::to_string(conn.strength) +
				   "\nplasticityRate: " + std::to_string(conn.plasticityRate) +
				   "\nplasticityThreshold: " + std::to_string(conn.plasticityThreshold) +
				   "\nreliability: " + std::to_string(conn.reliability);
		}

		if (scope_changed) scope.restore(scope_copy);
		return "Invalid scope";
	}

	std::string cmd_mutate()
	{
		unsigned mutationCount;
		try
		{
			mutationCount = cmd.size() < 2 ? 1 : std::stoul(cmd[1]);
		}
		catch (const std::exception &ex)
		{
			mutationCount = 1;
		}
		const auto &[env, net, neuron, conn] = targets();
		if (scope.active == "top")
		{
			switch (file.type())
			{
			case FileType::NONE:
				return "Nothing to mutate";
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment mutation not supported";
			case FileType::NETWORK:
				for (unsigned i = 0; i < mutationCount; i++)
					network.mutate();
				_updateFileData();
				return "Mutated network #" + file.data.network.id;
			}
		}

		if (scope.active == "network")
		{
			for (unsigned i = 0; i < mutationCount; i++)
				net->mutate();
			_updateFileData();
			return "Mutated network";
		}

		if (scope.active == "neuron")
		{
			for (unsigned i = 0; i < mutationCount; i++)
				neuron->mutate();
			_updateFileData();
			return "Mutated neuron";
		}

		if (scope.active == "connection")
		{
			for (unsigned i = 0; i < mutationCount; i++)
				conn->mutate();
			_updateFileData();
			return "Mutated connection";
		}

		return "Invalid scope";
	}

	std::string cmd_write()
	{
		std::string file_path = cmd.size() < 2 ? _path : cmd.at(1);
		file.write(file_path);
		return "Wrote to " + file_path;
	}

	static const inline std::map<std::string, std::function<std::string(Inspector *)>> commands{
		{"quit", &Inspector::cmd_quit},
		{"q", &Inspector::cmd_quit},
		{"help", &Inspector::cmd_help},
		{"h", &Inspector::cmd_help},
		{"print", &Inspector::cmd_print},
		{"p", &Inspector::cmd_print},
		{"scope:enter", &Inspector::cmd_scope_change},
		{">", &Inspector::cmd_scope_change},
		{"scope:leave", &Inspector::cmd_scope_change},
		{"<", &Inspector::cmd_scope_change},
		{"info", &Inspector::cmd_info},
		{"i", &Inspector::cmd_info},
		{"mutate", &Inspector::cmd_mutate},
		{"m", &Inspector::cmd_mutate},
		{"write", &Inspector::cmd_write},
		{"w", &Inspector::cmd_write},
	};

	std::tuple<Environment *, NeuralNetwork *, Neuron *, NeuronConnection *> targets()
	{
		Neuron *neuron = scope.values.at("neuron") == SIZE_MAX ? nullptr : &(network.neuron(scope.values.at("neuron")));
		return {
			nullptr,
			scope.values.at("network") == SIZE_MAX ? nullptr : &network,
			neuron,
			scope.values.at("connection") == SIZE_MAX || neuron == nullptr ? nullptr : &(neuron->outputs.at(scope.values.at("connection"))),
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
	Scope scope;

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
		if (_loaded)
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
		if (_loaded)
		{
			throw std::runtime_error("Already loaded");
		}

		file.read(_path);
		switch (file.type())
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
		if (!_loaded)
		{
			throw std::runtime_error("Not loaded");
		}

		_loaded = false;
	}

	std::string exec(std::string raw_command)
	{
		raw_cmd = raw_command;
		boost::split(cmd, raw_command, boost::is_any_of(" "));
		if (!commands.contains(cmd[0]))
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