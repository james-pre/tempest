#ifndef H_NeuralNetwork
#define H_NeuralNetwork

#include <vector>
#include <functional>
#include <cstdint>
#include <map>
#include <stdexcept>

typedef std::function<float(float)> ActivationFunction;

inline float reluDefaultActivation(float x)
{
	return (x > 0) ? x : 0;
}

enum class NeuronType
{
	NONE,
	TRANSITIONAL,
	INPUT,
	OUTPUT,
};

constexpr const int maxNeuronType = 4;

constexpr std::array<const char *, maxNeuronType> neuronTypes = {"none", "transitional", "input", "output"};

class NeuronConnection
{
public:
	size_t neuron;				   // ref to connected neuron
	float strength = 1;			   // strength of the connection (>1: excitatory, <1: inhibitory)
	float plasticityRate = 0;	   // how fast the strength changes
	float plasticityThreshold = 1; // the max strength
	float reliability = 1;		   // how reliable the passed value is

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

class Neuron
{
private:
	NeuronType _type;
	size_t _id;

public:
	NeuralNetwork *network;

	Neuron(NeuronType neuronType, NeuralNetwork &network, size_t id = SIZE_MAX);

	inline NeuronType type() const { return _type; }
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
			static_cast<uint16_t>(_type),
			outputsData};
	}
	static Neuron Deserialize(Serialized &data, NeuralNetwork &network);
};

class NeuralNetwork
{
private:
	size_t _id;

	// fix neurons' pointers to this network
	void _update()
	{
		for (auto &[id, neuron] : _neurons)
		{
			neuron.network = this;
		}
	}

public:
	NeuralNetwork(ActivationFunction activationFunction = reluDefaultActivation, size_t id = 0) : _id(id ? id : reinterpret_cast<std::uintptr_t>(&*this)), activationFunction(activationFunction) {}

	ActivationFunction activationFunction;

	std::map<size_t, Neuron> _neurons;

	Neuron &neuron(size_t id)
	{
		_update();
		return _neurons.at(id);
	}

	inline size_t size() const { return _neurons.size(); }

	size_t idOf(const Neuron *neuron) const
	{
		for (const auto &[id, n] : _neurons)
		{
			if (neuron == &n)
			{
				return id;
			}
		}
		throw std::runtime_error("Neuron not found in network");
	}

	inline size_t addNeuron(Neuron &neuron, size_t id = SIZE_MAX)
	{
		if (id == SIZE_MAX)
		{
			id = 0;
			while(hasNeuron(id))
			{
				id++;
			}
		}
		auto result = _neurons.insert(std::make_pair(id, neuron));
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
		return _neurons.count(id);
	}

	void removeNeuron(size_t id)
	{
		auto it = _neurons.find(id);
		if (it == _neurons.end())
		{
			throw std::out_of_range("Invalid neuron ID");
		}

		_neurons.erase(it);
	}

	std::vector<Neuron> neuronsOfType(NeuronType type)
	{
		std::vector<Neuron> ofType;
		for (auto &[id, neuron] : _neurons)
		{
			if (neuron.type() == type)
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
		uint64_t id;
		std::vector<Neuron::Serialized> neurons;
	};

	Serialized serialize() const
	{
		std::vector<Neuron::Serialized> neuronsData;

		for (auto &[id, neuron] : _neurons)
		{
			neuronsData.push_back(neuron.serialize());
		}

		return {
			_id,
			neuronsData};
	}
	static NeuralNetwork Deserialize(Serialized &data)
	{
		NeuralNetwork network(reluDefaultActivation, data.id);
		for (Neuron::Serialized &neuronData : data.neurons)
		{
			Neuron neuron = Neuron::Deserialize(neuronData, network);
			network.addNeuron(neuron);
		}

		return network;
	}
};

#endif