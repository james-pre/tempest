#ifndef H_inspector
#define H_inspector

#include <vector>
#include <string>
#include <map>
#include <numeric>
#include <cmath>
#include <memory>
#include "utils.hpp"
#include "File.hpp"

using namespace std::placeholders;

class Inspector : protected File
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

		std::vector<std::string> skip = {type() == FileType::NETWORK ? "environment" : ""};

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

		switch (type())
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

			scope.next(1, {type() == FileType::NETWORK ? "environment" : ""});
			scope[scope.active] = std::stoul(cmdv[i]);
			scope_changed = true;
		}
		if (scope.active == "top")
		{
			const std::string type_text = (header.type < maxFileType) ? fileTypes.at(header.type) : "Unknown (" + std::to_string(header.type) + ")";
			std::string data_text = "<error>";
			switch (type())
			{
			case FileType::NONE:
				data_text = "<none>";
				break;
			case FileType::PARTIAL:
			case FileType::FULL:
				data_text = "<not supported>";
				break;
			case FileType::NETWORK:
				if (network == nullptr)
				{
					return "No active network";
				}
				data_text = network->name + " (#" + std::to_string(network->id) + ")";
				break;
			}

			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting " + _path + ":" +
				   "\nmagic: " + magic() +
				   "\ntype: " + type_text +
				   "\nversion: " + std::to_string(version()) +
				   "\n\ndata: " + data_text;
		}

		if (scope.active == "environment")
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Environments not supported";
		}

		if (network == nullptr)
		{
			return "No active network";
		}

		if (scope.active == "network")
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting network #" + std::to_string(network->id) + ":" +
				   "\nname: " + network->name +
				   "\nactivation: " + network->activation +
				   "\nneurons: " + std::to_string(network->size());
		}

		size_t neuron_id = scope.at("neuron");
		if (!network->has(neuron_id))
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Neuron does not exist";
		}

		Neuron &neuron = network->get(neuron_id);
		if (scope.active == "neuron")
		{
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting neuron #" + std::to_string(neuron.id()) + ":" +
				   "\ntype: " + neuronTypes[static_cast<unsigned char>(neuron.type)] +
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
			Neuron::Connection &conn = neuron.outputs[conn_id];
			if (scope_changed)
				scope.restore(scope_copy);
			return "Inspecting connection #" + std::to_string(conn_id) + ":" +
				   "\nneuron: #" + std::to_string(conn.neuron) +
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

		BaseElement *target;

		if (scope.active == "top")
		{
			switch (type())
			{
			case FileType::NONE:
				return "Nothing to mutate";
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment mutation not supported";
			case FileType::NETWORK:
				target = static_cast<BaseElement *>(network);
			}
		}

		if (scope.active == "network")
			target = static_cast<BaseElement *>(network);
		if (scope.active == "neuron")
			target = static_cast<BaseElement *>(&network->get(scope.at("neuron")));
		if (scope.active == "connection")
			target = static_cast<BaseElement *>(&network->get(scope.at("neuron")).outputs.at(scope.at("connection")));

		if (target == nullptr)
		{
			return "Invalid target";
		}
		for (unsigned i = 0; i < mutationCount; i++)
			target->mutate();

		return "Mutated " + scope.active;
	}

	std::string cmd_write()
	{
		std::string file_path = cmdv.size() < 2 ? _path : cmdv[1];
		writePath(file_path);
		return "Wrote to " + file_path;
	}

	std::string cmd_create()
	{
		if (scope.active == "top")
		{
			switch (type())
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

		if (network == nullptr)
		{
			return "No active network";
		}
		if (scope.active == "network")
		{
			Neuron &n = network->create(NeuronType::TRANSITIONAL);

			return "Created neuron #" + std::to_string(n.id());
		}
		if (!network->has(scope.at("neuron")))
		{
			return "No active neuron";
		}
		Neuron &n = network->get(scope.at("neuron"));
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
			if (!network->has(output))
			{
				return "Specified output neuron does not exist";
			}
			n.connect(network->get(output));

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

		Reflectable *target;

		if (scope.active == "top")
		{
			switch (type())
			{
			case FileType::NONE:
				return "No data to " + std::string(cmdv[0] == "data:set" ? "modify" : "interact with");
			case FileType::PARTIAL:
			case FileType::FULL:
				return "Environment " + std::string(cmdv[0] == "data:set" ? "modification" : "data interaction") + " not supported";
			case FileType::NETWORK:
				target = static_cast<Reflectable *>(network);
			}
		}

		if (scope.active == "environment")
			return "Environment " + std::string(cmdv[0] == "data:set" ? "modification" : "data interaction") + " not supported";
		if (scope.active == "network")
			target = static_cast<Reflectable *>(network);
		if (scope.active == "neuron")
			target = static_cast<Reflectable *>(&network->get(scope.at("neuron")));
		if (scope.active == "connection")
			target = static_cast<Reflectable *>(&network->get(scope.at("neuron")).outputs.at(scope.at("connection")));

		if (target == nullptr)
		{
			return "No active " + scope.active;
		}

		if (!target->hasProperty(cmdv[1]))
		{
			return "Property \"" + cmdv[1] + "\" does not exist";
		}

		if (cmdv[0] == "data:get")
		{
			return cmdv[1] + " = " + target->getPropertyString(cmdv[1]);
		}

		try
		{
			target->setProperty(cmdv[1], cmdv[2]);
		}
		catch (const std::exception &ex)
		{
			return "Failed to set \"" + cmdv[1] + "\": " + ex.what();
		}
		return "Set \"" + cmdv[1] + "\" to \"" + target->getPropertyString(cmdv[1]) + "\"";
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

	std::string scope_stringifier(std::string name, size_t value) const
	{
		if (name == "network" && type() == FileType::NETWORK && network->id == value)
		{
			return network->name;
		}

		return Scope::stringify_default(name, value);
	}

	size_t scope_parser(std::string name, std::string value) const
	{
		if (name == "network" && type() == FileType::NETWORK && network->name == value)
		{
			return network->id;
		}

		return Scope::parse_default(name, value);
	}

	bool _loaded = false;
	std::string _path = "";
	std::vector<std::string> cmdv;

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

		readPath(_path);
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

		if (magic() != File::Magic)
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