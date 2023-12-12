#ifndef H_NeuralNetwork
#define H_NeuralNetwork

#include <vector>
#include <functional>
#include <cstdint>

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
	Serialized serialize();
	void mutate();
	static NeuronConnection Deserialize(Serialized &data, Neuron &neuron);

	inline bool operator==(const NeuronConnection& other) const { return this == &other; }
};

class NeuralNetwork;

class Neuron
{
private:
	NeuronType _type;
	std::vector<NeuronConnection> _outputs;
	size_t _id;

public:
	Neuron(NeuronType neuronType, NeuralNetwork &network, size_t id = 0);
	inline NeuronType type() const { return _type; }
	NeuralNetwork &network;
	inline const std::vector<NeuronConnection> outputs() const { return _outputs; }
	size_t id() const;
	NeuronConnection connect(Neuron &neuron);
	void unconnect(Neuron &neuron);
	void addConnection(NeuronConnection connection);
	void removeConnection(const NeuronConnection &connection);
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

	Serialized serialize();
	static Neuron Deserialize(Serialized &data, NeuralNetwork &network);
};

class NeuralNetwork
{
private:
	size_t _id;
	std::vector<Neuron> _neurons;
public:
	NeuralNetwork(ActivationFunction activationFunction = reluDefaultActivation, size_t id = 0);
	~NeuralNetwork();
	ActivationFunction activationFunction;
	
	Neuron &neuron(size_t id);
	inline size_t size() const { return _neurons.size(); }
	inline void addNeuron(Neuron &neuron) { _neurons.push_back(neuron); };
	size_t idOf(const Neuron &neuron) const;
    void removeNeuron(size_t id);
	std::vector<Neuron> neuronsOfType(NeuronType type);
	void update();
	void mutate();
	std::vector<float> processInput(std::vector<float> inputValues);

	struct Serialized
	{
		uint64_t id;
		uint64_t num_neurons;
		Neuron::Serialized *neurons;
	} __attribute__((packed));

	Serialized serialize();
	static NeuralNetwork Deserialize(Serialized &data);
};

#endif