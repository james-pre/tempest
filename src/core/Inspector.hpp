#ifndef H_inspector
#define H_inspector

#include <vector>
#include <string>
#include <map>
#include <numeric>
#include <cmath>
#include "utils.hpp"
#include "File.hpp"

class Inspector
{
public:
	class Scope
	{
	public:
		static const inline std::vector<std::string> scopes{
			"top",
			"environment",
			"network",
			"neuron",
			"connection",
		};

		std::string active = "top";

		std::map<std::string, size_t> values{
			{"top", 0},
			{"environment", SIZE_MAX},
			{"network", SIZE_MAX},
			{"neuron", SIZE_MAX},
			{"connection", SIZE_MAX},
		};

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

		void restore(Scope other)
		{
			active = other.active;
			for (const std::string &scope : scopes)
			{
				values[scope] = other.values[scope];
			}
		}

		void next(ptrdiff_t offset = 1, std::vector<std::string> skip = {})
		{
			active = nextTo(scopes, active, offset);
			while (std::find(skip.begin(), skip.end(), active) != skip.end() && active != scopes.front() && active != scopes.back())
			{
				active = nextTo(scopes, active, offset);
			}
		}

		void reset()
		{
			for (const std::string &scope : scopes)
			{
				values[scope] = scope == "top" ? 0 : SIZE_MAX;
			}
		}
	};

	struct Command
	{
		std::string id;
		std::vector<std::string> aliases;
		std::string description;
		std::function<std::string(Inspector *)> exec;

		static bool compare_id_size(const Command &a, const Command &b)
		{
			return a.id.size() < b.id.size();
		};
	};

private:
	std::string cmd_print() const
	{

		if (cmdv[1] == ".")
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

		bool entering = cmdv[0] == "scope:enter";
		auto offset = entering ? 1 : -1;

		scope.next(offset, {file.type() == FileType::NETWORK ? "environment" : ""});

		size_t target;
		if (entering)
		{
			if (scope.active == "network" && file.type() == FileType::NETWORK && cmdv[1] == "\%")
			{
				target = file.data.network.id;
			}
			else
			{
				target = std::stoul(cmdv[1]);
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
		const size_t max_size = std::max_element(commands.begin(), commands.end(), Command::compare_id_size)->id.size();

		std::string message = "Inspector commands:\n";
		for (const Command &cmd : commands)
		{
			const uint16_t tab_count = std::ceil((max_size - cmd.id.size() + 0.5) / 4);
			message += "\t" + cmd.id + std::string(tab_count, '\t') + cmd.description + "\n";
		}
		return message;
	}

	std::string cmd_info()
	{
		Scope scope_copy(scope);
		bool scope_changed = false;

		for (size_t i = 1; i < cmdv.size(); i++)
		{
			if (i >= Scope::scopes.size())
			{
				break;
			}

			scope.next(1, {file.type() == FileType::NETWORK ? "environment" : ""});
			scope.values[scope.active] = std::stoul(cmdv[i]);
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

			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting " + _path + ":" +
				   "\nmagic: " + file.magic() +
				   "\ntype: " + type +
				   "\nversion: " + std::to_string(file.version()) +
				   "\n\ndata: " + dataID;
		}

		if (scope.active == "environment")
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Environments not supported";
		}

		if (scope.active == "network")
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting network #" + std::to_string(file.data.network.id) + ":" +
				   "\nneurons: " + std::to_string(network.size());
		}

		if (scope.values.at("neuron") >= network.size())
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Neuron does not exist";
		}
		Neuron::Serialized neuron = file.data.network.neurons[scope.values.at("neuron")];
		if (scope.active == "neuron")
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting neuron #" + std::to_string(neuron.id) + ":" +
				   "\ntype: " + neuronTypes[neuron.type] +
				   "\noutputs: " + std::to_string(neuron.outputs.size());
		}

		if (scope.active == "connection")
		{
			if (scope.values.at("connection") >= neuron.outputs.size())
			{
				if (scope_changed)
					scope.restore(scope_copy);
				return "Connection does not exist";
			}
			NeuronConnection::Serialized conn = neuron.outputs[scope.values.at("connection")];
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting connection #" + std::to_string(scope.values.at("connection")) + ":" +
				   "\nto: #" + std::to_string(conn.neuron) +
				   "\nstrength: " + std::to_string(conn.strength) +
				   "\nplasticityRate: " + std::to_string(conn.plasticityRate) +
				   "\nplasticityThreshold: " + std::to_string(conn.plasticityThreshold) +
				   "\nreliability: " + std::to_string(conn.reliability);
		}

		if (scope_changed)
			scope.restore(scope_copy);
		return "Invalid scope";
	}

	std::string cmd_scope_reset()
	{
		scope.reset();
		return "Reset scope";
	}

	std::string cmd_mutate()
	{
		unsigned mutationCount;
		try
		{
			mutationCount = cmdv.size() < 2 ? 1 : std::stoul(cmdv[1]);
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
				_update();
				return "Mutated network #" + file.data.network.id;
			}
		}

		if (scope.active == "network")
		{
			for (unsigned i = 0; i < mutationCount; i++)
				net->mutate();
			_update();
			return "Mutated network";
		}

		if (scope.active == "neuron")
		{
			for (unsigned i = 0; i < mutationCount; i++)
				neuron->mutate();
			_update();
			return "Mutated neuron";
		}

		if (scope.active == "connection")
		{
			for (unsigned i = 0; i < mutationCount; i++)
				conn->mutate();
			_update();
			return "Mutated connection";
		}

		return "Invalid scope";
	}

	std::string cmd_write()
	{
		std::string file_path = cmdv.size() < 2 ? _path : cmdv[1];
		file.write(file_path);
		return "Wrote to " + file_path;
	}

	std::string cmd_create()
	{
		const auto &[env, net, neuron, conn] = targets();
		if (scope.active == "top")
		{
			switch (file.type())
			{
			case FileType::NONE:
				return "Nothing to create";
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment creation not supported";
			case FileType::NETWORK:
				return "Network creation not supported";
			}
		}

		if (net == nullptr)
		{
			return "No active network";
		}
		if (scope.active == "network")
		{
			Neuron neuron = net->createNeuron(NeuronType::TRANSITIONAL);
			_update();
			return "Created neuron #" + std::to_string(neuron.id());
		}
		if (neuron == nullptr)
		{
			return "No active neuron";
		}
		if (scope.active == "neuron")
		{
			if (cmdv.size() < 2)
			{
				return "No output neuron specified";
			}
			size_t output;
			try
			{
				output = std::stoul(cmdv[1]);
			}
			catch (const std::exception &ex)
			{
				return "Invalid output";
			}
			if (!net->hasNeuron(output))
			{
				return "Specified output neuron does not exist";
			}
			neuron->connect(net->neuron(output));
			_update();
			return "Created connection";
		}

		if (scope.active == "connection")
		{
			return "Nothing to create";
		}

		return "Invalid scope";
	}

	std::string cmd_set()
	{
		if (cmdv.size() < 3)
		{
			return "Missing arguments";
		}
		const auto &[env, net, neuron, conn] = targets();
		if (scope.active == "top")
		{
			switch (file.type())
			{
			case FileType::NONE:
				return "Nothing to modify";
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment modification not supported";
			case FileType::NETWORK:
				return "Network modification not supported";
			}
		}

		if (scope.active == "network")
		{
			if (net == nullptr)
			{
				return "No active network";
			}

			if (cmdv[1] == "")
			{
			}
			_update();
			return "";
		}
		if (neuron == nullptr)
		{
			return "No active neuron";
		}
		if (scope.active == "neuron")
		{
			if (cmdv.size() < 2)
			{
				return "No output neuron specified";
			}
			size_t output;
			try
			{
				output = std::stoul(cmdv[1]);
			}
			catch (const std::exception &ex)
			{
				return "Invalid output";
			}
			if (!net->hasNeuron(output))
			{
				return "Specified output neuron does not exist";
			}
			neuron->connect(net->neuron(output));
			_update();
			return "Created connection";
		}

		if (scope.active == "connection")
		{
			return "Nothing to create";
		}

		return "Invalid scope";
	}

	static const inline std::vector<Command> commands = {
		{"quit", {"q", "exit"}, "Quits the inspector", &Inspector::cmd_quit},
		{"help", {"h"}, "Displays this help message or the description of a command", &Inspector::cmd_help},
		{"print", {"p"}, "(ununsed)", &Inspector::cmd_print},
		{"scope:enter", {">"}, "Enter a deeper scope", &Inspector::cmd_scope_change},
		{"scope:leave", {"<"}, "Leave a deeper scope", &Inspector::cmd_scope_change},
		{"scope:reset", {}, "Reset scopes", &Inspector::cmd_scope_reset},
		{"info", {"i"}, "Display info about the current object", &Inspector::cmd_info},
		{"mutate", {"m"}, "Mutate the current object", &Inspector::cmd_mutate},
		{"write", {"w"}, "Write changes made to the file to disk", &Inspector::cmd_write},
		{"create", {"c"}, "Create a new child object of the current object", &Inspector::cmd_create},
		{"set", {"s"}, "Set a value on the current object", &Inspector::cmd_set},
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

	// update file data
	void _update()
	{
		file.data.network = network.serialize();
	}

	bool _loaded = false;
	std::string _path = "";
	std::vector<std::string> cmdv;

	File file;
	NeuralNetwork network;

public:
	Scope scope;

	bool loaded() const
	{
		return _loaded;
	}

	const std::string &path() const
	{
		return _path;
	}

	void path(const std::string &path)
	{
		if (_loaded)
		{
			throw std::runtime_error("Can not change path while file is loaded");
		}
		_path = path;
	}

	const std::vector<std::string> &command() const
	{
		return cmdv;
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

	std::string exec(const std::string raw_command)
	{
		return exec(split(raw_command, " "));
	}

	std::string exec(std::vector<std::string> command)
	{
		cmdv = command;
		for (const Command &cmd : commands)
		{
			for (const std::string &alias : cmd.aliases)
			{
				if (cmdv[0] == alias)
				{
					cmdv[0] = cmd.id;
					break;
				}
			}

			if (cmdv[0] == cmd.id)
			{
				return cmd.exec(this);
			}
		}

		return "Command does not exist";
	}
};

#endif