#ifndef H_NeuralNetwork
#define H_NeuralNetwork

#include <vector>
#include <functional>
#include <cstdint>
#include <map>
#include <stdexcept>
#include "utils.hpp"
#include "generic.hpp"

#define COPY_WARNING "Copy not allowed"

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

class NeuralNetwork;

class Neuron : public BaseElement
{
public:
	struct ConnectionData
	{
		size_t neuron = SIZE_MAX;	   // ref to connected neuron
		float strength = 0.75;			   // strength of the connection (>1: excitatory, <1: inhibitory)
		float plasticityRate = 0;	   // how fast the strength changes
		float plasticityThreshold = 1; // the max strength
		float reliability = 1;		   // how reliable the passed value is
	} __attribute__((packed));

	class Connection : public ConnectionData, public BaseElement
	{

	public:
		REFLECT(neuron, strength, plasticityRate, plasticityThreshold, reliability);

		Connection(
			size_t _neuron = SIZE_MAX,
			float _strength = 0.75,
			float _plasticityRate = 0,
			float _plasticityThreshold = 1,
			float _reliability = 1)
		{
			neuron = _neuron;
			strength = _strength;
			plasticityRate = _plasticityRate;
			plasticityThreshold = _plasticityThreshold;
			reliability = _reliability;
		}

		Connection(ConnectionData &data)
		{
			neuron = data.neuron;
			strength = data.strength;
			plasticityRate = data.plasticityRate;
			plasticityThreshold = data.plasticityThreshold;
			reliability = data.reliability;
		}

		// Todo: This should be changed to properly account for the different attributes
		void mutate()
		{
			float newValue = (rand_seeded<float>() - .5) * rand_seeded<float>() / 2;
			switch (rand_seeded<int>() % 5)
			{
			case 0:
				return;
			case 1:
				strength += newValue;
				return;
			case 2:
				plasticityRate += newValue;
				return;
			case 3:
				plasticityThreshold += newValue;
				return;
			case 4:
				reliability += newValue;
				return;
			}
		}

		bool operator==(const Connection &other) const
		{
			return neuron == other.neuron &&
				   strength == other.strength &&
				   plasticityRate == other.plasticityRate &&
				   plasticityThreshold == other.plasticityThreshold &&
				   reliability == other.reliability;
		}
	};

	size_t _id;
	NeuralNetwork *network;
	NeuronType type;
	std::vector<Connection> outputs{};

	size_t id() const;

	REFLECT(type, value)

	Neuron(NeuronType neuronType = NeuronType::TRANSITIONAL, NeuralNetwork *network = nullptr, size_t id = 0);

	__attribute__((warning(COPY_WARNING)))
	Neuron(const Neuron &other)
	{
		from(other);
	}

	__attribute__((warning(COPY_WARNING)))
	Neuron &
	operator=(const Neuron &other)
	{
		if (this == &other)
		{
			return *this;
		}
		from(other);
		return *this;
	}

	void from(const Neuron &other)
	{
		outputs.resize(other.outputs.size());
		_id = other._id;
		network = other.network;
		type = other.type;
		outputs = other.outputs;
		value = other.value;
	}

	void addConnection(Connection connection)
	{
		outputs.push_back(connection);
	}

	void addConnection(ConnectionData connection)
	{
		outputs.push_back(Connection(connection));
	}

	void removeConnection(const Connection &connection)
	{
		std::remove(outputs.begin(), outputs.end(), connection);
	}

	Connection connect(Neuron &neuron)
	{
		Connection connection = {neuron.id()};
		addConnection(connection);
		return connection;
	}

	void unconnect(Neuron &neuron)
	{
		const auto it = std::find_if(outputs.begin(), outputs.end(), [&](Connection &conn)
									 { return conn.neuron == neuron.id(); });

		if (it == outputs.end())
		{
			throw std::runtime_error("Neuron is not connected");
		}

		removeConnection(*it);
	}

	float value = 0.5;

	void update(unsigned max_depth = 1000);
	void mutate(BaseElement::MutationOptions options);
};

class NeuralNetwork : public std::map<size_t, Neuron>, public BaseElement
{
public:
	using Map = std::map<size_t, Neuron>;
	using Activation = std::function<float(float)>;
	using Values = std::vector<float>;
	using NeuronV = std::vector<std::reference_wrapper<Neuron>>;
	using UpdateCallback = std::function<void(const Values)>;

	class activations
	{
	public:
		static float relu(float x)
		{
			return (x > 0) ? x : 0;
		}
	};

protected:
	// fix neurons' pointers to this network
	void _update()
	{
		for (auto &[id, neuron] : *this)
		{
			neuron.network = this;
		}
	}

	void _notify()
	{
		if(!runCallback)
		{
			return;
		}

		runCallback(output_values());
	}

	unsigned _max_depth = 1;

	UpdateCallback runCallback;

	friend class Neuron;

	NeuronV ofType(NeuronType type)
	{
		NeuronV ofType;
		for (auto &[id, neuron] : *this)
		{
			if (neuron.type == type)
			{
				//neuron.network = this;
				ofType.push_back(std::ref(neuron));
			}
		}

		return ofType;
	}

public:
	const static inline std::map<std::string, Activation> activations{
		{"relu", &activations::relu},
	};

	size_t id;
	std::string name;
	std::string activation;

	const Activation &activationFunction() const
	{
		if(!activations.contains(activation))
		{
			throw new std::runtime_error("activation function does not exist");
		}
		return activations.at(activation);
	}

	BaseElement::MutationOptions mutationOptions;

	REFLECT(name, activation)

	NeuralNetwork(std::string activation = "relu", std::string name = "", size_t id = 0) : id(id != 0 ? id : reinterpret_cast<std::uintptr_t>(&*this)), name(!name.empty() ? name : "default"), activation(activation)
	{
		if (!activations.contains(activation))
		{
			throw std::runtime_error("Invalid activation function: \"" + activation + "\"");
		}
	}

	__attribute__((warning(COPY_WARNING)))
	NeuralNetwork(const NeuralNetwork &other) : std::map<size_t, Neuron>(other)
	{
		from(other);
	}

	__attribute__((warning(COPY_WARNING)))
	NeuralNetwork &
	operator=(const NeuralNetwork &other)
	{
		if (this == &other)
		{
			return *this;
		}

		from(other);

		return *this;
	}

	// deep copy data from another network
	void from(const NeuralNetwork &other)
	{
		id = other.id;
		name = other.name;
		activation = other.activation;
		for (const auto &[id, neuron] : other)
		{
			Neuron copied;
			copied.from(neuron);
			copied.network = this;
			this->insert_or_assign(id, neuron);
		}
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

	size_t next_id(size_t id = 0)
	{
		while (has(id))
		{
			id++;
		}
		return id;
	}

	Neuron &get(size_t id)
	{
		_update();
		return at(id);
	}

	size_t add(Neuron &neuron)
	{
		size_t id = next_id(neuron._id);
		neuron._id = id;

		if (has(id))
		{
			throw std::runtime_error("Neuron with the same ID already exists");
		}
		neuron.network = this;
		insert_or_assign(id, neuron);
		return id;
	}

	Neuron &create(NeuronType type)
	{
		Neuron _neuron(type, this);
		return get(add(_neuron));
	}

	bool has(size_t id)
	{
		return contains(id);
	}

	void remove(size_t id)
	{
		auto it = find(id);
		if (it == end())
		{
			throw std::out_of_range("Invalid neuron ID");
		}
		erase(it);
	}

	NeuronV inputs()
	{
		return ofType(NeuronType::INPUT);
	}

	Values input_values()
	{
		Values inputValues;
		for (Neuron &input : inputs())
		{
			inputValues.push_back(input.value);
		}
		return inputValues;
	}

	void input_values(Values values)
	{
		NeuronV _inputs = inputs();
		for (size_t i = 0; i < values.size(); ++i)
		{
			Neuron &n = _inputs[i];
			n.value = values[i];
		}
	}

	NeuronV outputs()
	{
		return ofType(NeuronType::OUTPUT);
	}

	Values output_values()
	{
		Values outputValues;
		for (Neuron &output : outputs())
		{
			outputValues.push_back(output.value);
		}
		return outputValues;
	}

	void update(unsigned max_depth = 1000)
	{
		_max_depth = max_depth;
		for (Neuron &inputNeuron : inputs())
		{
			inputNeuron.update(max_depth);
		}
	}

	void mutate()
	{

		float random = rand_seeded<float>();

		size_t target = rand_seeded<unsigned>() % size();
		if (random < .6)
		{
			get(target).mutate(mutationOptions);
			return;
		}

		if (random < 0.75 && has(target))
		{
			remove(target);
			return;
		}

		create(NeuronType::TRANSITIONAL);
	}

	Values run(const Values inputValues, unsigned max_depth = 1000, UpdateCallback onUpdate = nullptr)
	{
		if (inputValues.size() != inputs().size())
		{
			throw new std::invalid_argument("Input size does not match the number of input neurons.");
		}

		runCallback = onUpdate;
		input_values(inputValues);
		update(max_depth);
		return output_values();
	}
};

#endif