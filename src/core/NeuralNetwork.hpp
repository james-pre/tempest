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

class Neuron;

class NeuronConnection
{
public:
	Neuron *neuron;				   // ref to connected neuron
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

	Serialized serialize() const;

	void mutate();

	static NeuronConnection Deserialize(Serialized &data, Neuron &neuron)
	{
		return NeuronConnection{
			&neuron,
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
	std::vector<NeuronConnection> _outputs;
	size_t _id;
public:
	NeuralNetwork &network;
	Neuron(NeuronType neuronType, NeuralNetwork &network, size_t id = SIZE_MAX);

	inline NeuronType type() const { return _type; }
	inline const std::vector<NeuronConnection> outputs() const { return _outputs; }
	size_t id() const;

	void addConnection(NeuronConnection connection)
	{
		_outputs.push_back(connection);
	}

	void removeConnection(const NeuronConnection &connection)
	{
		std::remove(_outputs.begin(), _outputs.end(), connection);
	}

	NeuronConnection connect(Neuron &neuron)
	{
		NeuronConnection connection = {&neuron};
		addConnection(connection);
		return connection;
	}

	void unconnect(Neuron &neuron)
	{
		const auto it = std::find_if(_outputs.begin(), _outputs.end(), [&](NeuronConnection &conn)
									 { return conn.neuron->id() == neuron.id(); });

		if (it == _outputs.end())
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
		uint64_t num_outputs;
		NeuronConnection::Serialized *outputs;
	} __attribute__((packed));

	Serialized serialize() const
	{
		std::vector<NeuronConnection::Serialized> serializedOutputs;
		for(const NeuronConnection &conn : outputs())
		{
			serializedOutputs.push_back(conn.serialize());
		}

		return {
			id(),
			static_cast<uint16_t>(_type),
			_outputs.size(),
			serializedOutputs.data()
		};
	}
	static Neuron Deserialize(Serialized &data, NeuralNetwork &network);
};

class NeuralNetwork
{
private:
	size_t _id;
	std::map<size_t, Neuron> _neurons;

public:
	NeuralNetwork(ActivationFunction activationFunction = reluDefaultActivation, size_t id = 0) : _id(id ? id : reinterpret_cast<std::uintptr_t>(&*this)), activationFunction(activationFunction) {}

	ActivationFunction activationFunction;

	Neuron &neuron(size_t id)
	{
		auto it = _neurons.find(id);
		if(it == _neurons.end()) {
			throw std::out_of_range("Invalid neuron ID");
		}
	
		return it->second;
	}

	inline size_t size() const { return _neurons.size(); }
	inline void addNeuron(Neuron &neuron, size_t id = SIZE_MAX)
	{
		if (id == SIZE_MAX)
		{
			id = size();
		}

		auto result = _neurons.insert(std::make_pair(id, std::move(neuron)));
		if (!result.second)
		{
			throw std::runtime_error("Neuron with the same ID already exists");
		}
	}

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

	void removeNeuron(size_t id)
	{
		auto it = _neurons.find(id);
		if (it == _neurons.end()) {
			throw std::out_of_range("Invalid neuron ID");
		}
		
		_neurons.erase(it);
	}

	std::vector<Neuron> neuronsOfType(NeuronType type)
	{
		std::vector<Neuron> ofType;
		for (const auto &[id, neuron] : _neurons)
		{
			if (neuron.type() == type)
			{
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
		uint64_t num_neurons;
		Neuron::Serialized *neurons;
	} __attribute__((packed));

	Serialized serialize()
	{
		std::vector<Neuron::Serialized> serializedNeurons;

		for (const auto &[id, neuron] : _neurons)
		{
			serializedNeurons.push_back(neuron.serialize());
		}

		return {
			_id,
			size(),
			serializedNeurons.data()
		};
	}
	static NeuralNetwork Deserialize(Serialized &data)
	{
		NeuralNetwork network(reluDefaultActivation, data.id);
		for(size_t i = 0; i < data.num_neurons; i++)
		{
			Neuron neuron = Neuron::Deserialize(data.neurons[i], network);
			network.addNeuron(neuron);
		}

		return network;
	}
};

#endif