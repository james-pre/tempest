#ifndef H_NeuralNetwork
#define H_NeuralNetwork

#include <vector>
#include <functional>
#include <cstdint>
#include <map>
#include <stdexcept>
#include "utils.hpp"

enum class NeuronType
{
	NONE,
	TRANSITIONAL,
	INPUT,
	OUTPUT,
};

namespace std
{
	string to_string(string &val)
	{
		return val;
	}
	string to_string(NeuronType val)
	{
		return to_string(static_cast<unsigned int>(val));
	}
}

constexpr const int maxNeuronType = 4;

constexpr std::array<const char *, maxNeuronType> neuronTypes = {"none", "transitional", "input", "output"};

class NeuronConnection : public Reflectable
{

public:
	size_t neuron;				   // ref to connected neuron
	float strength = 1;			   // strength of the connection (>1: excitatory, <1: inhibitory)
	float plasticityRate = 0;	   // how fast the strength changes
	float plasticityThreshold = 1; // the max strength
	float reliability = 1;		   // how reliable the passed value is

	REFLECT(neuron, strength, plasticityRate, plasticityThreshold, reliability);

	NeuronConnection(
		size_t neuron,
		float strength = 1,
		float plasticityRate = 0,
		float plasticityThreshold = 1,
		float reliability = 1) : neuron(neuron),
								 strength(strength),
								 plasticityRate(plasticityRate),
								 plasticityThreshold(plasticityThreshold),
								 reliability(reliability)
	{
	}

	struct Serialized
	{
		uint64_t neuron;
		float strength;
		float plasticityRate;
		float plasticityThreshold;
		float reliability;
	} __attribute__((packed));

	Serialized serialize() const
	{
		return Serialized{
			neuron,
			strength,
			plasticityRate,
			plasticityThreshold,
			reliability,
		};
	}

	void mutate();

	static NeuronConnection Deserialize(Serialized &data)
	{
		return NeuronConnection{
			data.neuron,
			data.strength,
			data.plasticityRate,
			data.plasticityThreshold,
			data.reliability,
		};
	}

	bool operator==(const NeuronConnection &other) const { return this == &other; }
};

class NeuralNetwork;

class Neuron : public Reflectable
{
private:
	size_t _id;

public:
	NeuralNetwork *network;
	NeuronType type;

	REFLECT(type, value)

	Neuron(NeuronType neuronType, NeuralNetwork &network, size_t id = SIZE_MAX);

	std::vector<NeuronConnection> outputs;
	size_t id() const;

	void addConnection(NeuronConnection connection)
	{
		outputs.push_back(connection);
	}

	void removeConnection(const NeuronConnection &connection)
	{
		std::remove(outputs.begin(), outputs.end(), connection);
	}

	NeuronConnection connect(Neuron &neuron)
	{
		NeuronConnection connection = {neuron.id()};
		addConnection(connection);
		return connection;
	}

	void unconnect(Neuron &neuron)
	{
		const auto it = std::find_if(outputs.begin(), outputs.end(), [&](NeuronConnection &conn)
									 { return conn.neuron == neuron.id(); });

		if (it == outputs.end())
		{
			throw std::runtime_error("Neuron is not connected");
		}

		removeConnection(static_cast<NeuronConnection>(*it));
	}

	float value = 0;

	void update();
	void mutate();

	struct Serialized
	{
		uint64_t id;
		uint16_t type;
		std::vector<NeuronConnection::Serialized> outputs;
	};

	Serialized serialize() const
	{
		std::vector<NeuronConnection::Serialized> outputsData;
		for (const NeuronConnection &conn : outputs)
		{
			outputsData.push_back(conn.serialize());
		}

		return {
			id(),
			static_cast<uint16_t>(type),
			outputsData};
	}
	static Neuron Deserialize(Serialized &data, NeuralNetwork &network);
};

class NeuralNetwork : protected std::map<size_t, Neuron>, public Reflectable
{

public:
	using Activation = std::function<float(float)>;

	class activations
	{
	public:
		static float relu(float x)
		{
			return (x > 0) ? x : 0;
		}
	};

private:
	size_t _id;

protected:
	// fix neurons' pointers to this network
	void _update()
	{
		for (auto &[id, neuron] : *this)
		{
			neuron.network = this;
		}
	}

public:
	const static inline std::map<std::string, Activation> activations{
		{"relu", &activations::relu}};

	using std::map<size_t, Neuron>::size, std::map<size_t, Neuron>::begin, std::map<size_t, Neuron>::end;

	std::string activation;

	const Activation &activationFunction()
	{
		return activations.at(activation);
	}

	REFLECT(activation)

	NeuralNetwork(std::string activation = "relu", size_t id = 0) : _id(id ? id : reinterpret_cast<std::uintptr_t>(&*this)), activation(activation)
	{
		if (!activations.contains(activation))
		{
			throw std::runtime_error("Invalid activation function: \"" + activation + "\"");
		}
	}

	Neuron &neuron(size_t id)
	{
		_update();
		return at(id);
	}

	size_t idOf(const Neuron *neuron) const
	{
		for (const auto &[id, n] : *this)
		{
			if (neuron == &n)
			{
				return id;
			}
		}
		throw std::runtime_error("Neuron not found in network");
	}

	size_t addNeuron(Neuron &neuron, size_t id = SIZE_MAX)
	{
		if (id == SIZE_MAX)
		{
			id = 0;
			while (hasNeuron(id))
			{
				id++;
			}
		}
		auto result = insert(std::make_pair(id, neuron));
		if (!result.second)
		{
			throw std::runtime_error("Neuron with the same ID already exists");
		}
		return id;
	}

	Neuron &createNeuron(NeuronType type)
	{
		Neuron _neuron(type, *this);
		return neuron(addNeuron(_neuron));
	}

	bool hasNeuron(size_t id)
	{
		_update();
		return count(id);
	}

	void removeNeuron(size_t id)
	{
		auto it = find(id);
		if (it == end())
		{
			throw std::out_of_range("Invalid neuron ID");
		}

		erase(it);
	}

	std::vector<Neuron> neuronsOfType(NeuronType type)
	{
		std::vector<Neuron> ofType;
		for (auto &[id, neuron] : *this)
		{
			if (neuron.type == type)
			{
				neuron.network = this;
				ofType.push_back(neuron);
			}
		}

		return ofType;
	}

	void update();
	void mutate();
	std::vector<float> processInput(std::vector<float> inputValues);

	struct Serialized
	{
		size_t id;
		std::string activation;
		std::vector<Neuron::Serialized> neurons;
	};

	Serialized serialize() const
	{
		std::vector<Neuron::Serialized> neuronsData;

		for (auto &[id, neuron] : *this)
		{
			neuronsData.push_back(neuron.serialize());
		}

		return {_id, activation, neuronsData};
	}
	static NeuralNetwork Deserialize(Serialized &data)
	{
		NeuralNetwork network(data.activation, data.id);
		for (Neuron::Serialized &neuronData : data.neurons)
		{
			Neuron neuron = Neuron::Deserialize(neuronData, network);
			network.addNeuron(neuron);
		}

		return network;
	}
};

#endif