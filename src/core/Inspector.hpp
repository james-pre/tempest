#ifndef H_inspector
#define H_inspector

#include <vector>
#include <string>
#include <map>
#include <numeric>
#include <cmath>
#include "utils.hpp"
#include "File.hpp"

using namespace std::placeholders;

class Inspector
{
public:
	class Scope : public std::map<std::string, size_t>
	{
	public:
		using stringifier = std::function<std::string(std::string, size_t)>;

		static std::string stringify_default([[maybe_unused]] const std::string &name, const size_t value)
		{
			return std::to_string(value);
		}

		using parser = std::function<size_t(std::string, std::string)>;

		static size_t parse_default([[maybe_unused]] const std::string &name, const std::string &value)
		{
			return std::stoul(value);
		}

		using map = std::map<std::string, size_t>;

		static const inline std::vector<std::string> scopes{
			"top",
			"environment",
			"network",
			"neuron",
			"connection",
		};

		static const inline std::string delimiter = "/";

		std::string active = "top";

		Scope(map values = {}) : map({
									 {"top", 0},
									 {"environment", SIZE_MAX},
									 {"network", SIZE_MAX},
									 {"neuron", SIZE_MAX},
									 {"connection", SIZE_MAX},
								 })
		{
			from(values, true);
		}

		void restore(Scope other)
		{
			active = other.active;
			for (const std::string &scope : scopes)
			{
				(*this)[scope] = other[scope];
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
				(*this)[scope] = scope == "top" ? 0 : SIZE_MAX;
			}
		}

		void from(map new_values, bool partial = false)
		{
			for (const std::string &scope : scopes)
			{
				if (!new_values.contains(scope) && partial)
				{
					continue;
				}
				(*this)[scope] = new_values[scope];
			}
		}

		std::string stringify(const stringifier &stringify = stringify_default) const
		{
			return std::accumulate(
				std::next(scopes.begin()),
				std::next(std::find(scopes.begin(), scopes.end(), active)),
				std::string(""),
				[&](const std::string a, const std::string b)
				{
					return at(b) == SIZE_MAX ? a : a + delimiter + stringify(b, at(b));
				});
		}

		/*
			Parses a scope path (e.g. a/b/c)
			@param path The scope path
			@param relative Whether to resolve the scope path starting from the active scope
		*/
		size_t parse(const std::string &path, bool relative = true, std::vector<std::string> skip = {}, const parser parse = parse_default)
		{
			std::vector<std::string> tokens = split(path, delimiter);
			size_t offset = relative ? std::distance(scopes.begin(), std::find(scopes.begin(), scopes.end(), active)) : 0;
			size_t end = std::min(offset + tokens.size(), scopes.size());
			for (size_t i = offset; i < end; i++)
			{
				next(1, skip);
				(*this)[active] = parse(active, tokens[i - offset]);
			}
			return tokens.size();
		}

		static Scope Parse(const std::string &path, bool relative = true, std::vector<std::string> skip = {}, const parser parse = parse_default)
		{
			Scope scope;
			scope.parse(path, relative, skip, parse);
			return scope;
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
	std::string cmd_scope_change()
	{

		bool entering = cmdv[0] == "scope:enter";
		ptrdiff_t offset = -1;

		std::vector<std::string> skip = {file.type() == FileType::NETWORK ? "environment" : ""};

		if (entering)
		{
			if (cmdv[1].empty())
			{
				return "Missing deeper scope";
			}
			offset = parse_scope(cmdv[1], true, skip);
		}
		else
		{
			scope.next(offset, skip);
		}

		switch (file.type())
		{
		case FileType::NONE:
			return "No file data";
		case FileType::PARTIAL:
		case FileType::FULL:
			return "Environments not supported";
		case FileType::NETWORK:
			return "Changed scope to \"" + scope.active + "\"" + (entering ? " (#" + std::to_string(scope[scope.active]) + ")" : "");
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
			scope[scope.active] = std::stoul(cmdv[i]);
			scope_changed = true;
		}
		if (scope.active == "top")
		{
			const std::string type = (file.header.type < maxFileType) ? fileTypes.at(file.header.type) : "Unknown (" + std::to_string(file.header.type) + ")";
			std::string data_text = "<error>";
			switch (file.type())
			{
			case FileType::NONE:
				data_text = "<none>";
				break;
			case FileType::PARTIAL:
			case FileType::FULL:
				data_text = "<not supported>";
				break;
			case FileType::NETWORK:
				data_text = file.data.network.name + " (#" + std::to_string(file.data.network.id) + ")";
				break;
			}

			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting " + _path + ":" +
				   "\nmagic: " + file.magic() +
				   "\ntype: " + type +
				   "\nversion: " + std::to_string(file.version()) +
				   "\n\ndata: " + data_text;
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
				   "\nname: " + network.name +
				   "\nactivation: " + network.activation +
				   "\nneurons: " + std::to_string(network.size());
		}

		size_t neuron_id = scope.at("neuron");
		if (neuron_id >= network.size())
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Neuron does not exist";
		}
		Neuron::Serialized neuron = file.data.network.neurons[neuron_id];
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
			size_t conn_id = scope.at("connection");
			if (conn_id >= neuron.outputs.size())
			{
				if (scope_changed)
					scope.restore(scope_copy);
				return "Connection does not exist";
			}
			NeuronConnection::Serialized conn = neuron.outputs[conn_id];
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting connection #" + std::to_string(conn_id) + ":" +
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

	std::string cmd_data()
	{
		if (cmdv.size() < (cmdv[0] == "data:set" ? 3 : 2))
		{
			return "Missing arguments";
		}
		const auto &[env, net, neuron, conn] = targets();
		Reflectable *target = nullptr;

		if (scope.active == "top")
		{
			switch (file.type())
			{
			case FileType::NONE:
				return "No data to " + std::string(cmdv[0] == "data:set" ? "modify" : "interact with");
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment " + std::string(cmdv[0] == "data:set" ? "modification" : "data interaction") + " not supported";
			case FileType::NETWORK:
				target = static_cast<Reflectable *>(net);
			}
		}

		if (scope.active == "environment")
			return "Environment " + std::string(cmdv[0] == "data:set" ? "modification" : "data interaction") + " not supported";
		if (scope.active == "network")
			target = static_cast<Reflectable *>(net);
		if (scope.active == "neuron")
			target = static_cast<Reflectable *>(neuron);
		if (scope.active == "connection")
			target = static_cast<Reflectable *>(conn);

		if (target == nullptr)
		{
			return "No active " + scope.active;
		}

		if (!target->has(cmdv[1]))
		{
			return "Property \"" + cmdv[1] + "\" does not exist";
		}

		if (cmdv[0] == "data:get")
		{
			return cmdv[1] + " = " + target->get_string(cmdv[1]);
		}

		try
		{
			target->set(cmdv[1], cmdv[2]);
			_update();
		}
		catch (const std::exception &ex)
		{
			return "Failed to set \"" + cmdv[1] + "\": " + ex.what();
		}
		return "Set \"" + cmdv[1] + "\" to \"" + target->get_string(cmdv[1]) + "\"";
	}

	static const inline std::vector<Command> commands = {
		{"quit", {"q", "exit"}, "Quits the inspector", &Inspector::cmd_quit},
		{"help", {"h"}, "Displays this help message or the description of a command", &Inspector::cmd_help},
		{"scope:enter", {">"}, "Enter a deeper scope", &Inspector::cmd_scope_change},
		{"scope:leave", {"<"}, "Leave a deeper scope", &Inspector::cmd_scope_change},
		{"scope:reset", {}, "Reset scopes", &Inspector::cmd_scope_reset},
		{"info", {"i"}, "Display info about the current object", &Inspector::cmd_info},
		{"mutate", {"m"}, "Mutate the current object", &Inspector::cmd_mutate},
		{"write", {"w"}, "Write changes made to the file to disk", &Inspector::cmd_write},
		{"create", {"c"}, "Create a new child object of the current object", &Inspector::cmd_create},
		{"data:set", {"set", "s"}, "Set a value on the current object", &Inspector::cmd_data},
		{"data:get", {"get", "g", "print", "p"}, "Get a value on the current object", &Inspector::cmd_data},
	};

	std::tuple<Environment *, NeuralNetwork *, Neuron *, NeuronConnection *> targets()
	{
		Neuron *neuron = scope.at("neuron") == SIZE_MAX ? nullptr : &(network.neuron(scope.at("neuron")));
		return {
			nullptr,
			scope.at("network") == SIZE_MAX ? nullptr : &network,
			neuron,
			scope.at("connection") == SIZE_MAX || neuron == nullptr ? nullptr : &(neuron->outputs.at(scope.at("connection"))),
		};
	}

	std::string scope_stringifier(std::string name, size_t value) const
	{
		if (name == "network" && file.type() == FileType::NETWORK && file.data.network.id == value)
		{
			return file.data.network.name;
		}

		return Scope::stringify_default(name, value);
	}

	size_t scope_parser(std::string name, std::string value) const
	{
		if (name == "network" && file.type() == FileType::NETWORK && file.data.network.name == value)
		{
			return file.data.network.id;
		}

		return Scope::parse_default(name, value);
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

	const std::string &command() const
	{
		return cmdv[0];
	}

	const std::vector<std::string> &commandv() const
	{
		return cmdv;
	}

	size_t parse_scope(const std::string &path, bool relative = true, std::vector<std::string> skip = {})
	{
		return scope.parse(path, relative, skip, std::bind(&Inspector::scope_parser, this, _1, _2));
	}

	std::string stringify_scope()
	{
		return scope.stringify(std::bind(&Inspector::scope_stringifier, this, _1, _2));
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
		if (!_loaded)
		{
			return "File not loaded";
		}

		if (file.magic() != File::Magic)
		{
			return "File invalid";
		}
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