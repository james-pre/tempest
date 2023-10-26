#ifndef H_NeuralNetwork
#define H_NeuralNetwork

#include <vector>
#include <functional>

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

struct NeuronConnection
{
	Neuron *neuron;				   // ref to connected neuron
	float strength = 1;			   // strength of the connection (>1: excitatory, <1: inhibitory)
	float plasticityRate = 0;	   // how fast the strength changes
	float plasticityThreshold = 1; // the max strength
	float reliability = 1;		   // how reliable the passed value is

	struct Serialized {
		unsigned long int neuron;
		float strength;
		float plasticityRate;
		float plasticityThreshold;
		float reliability;
	};
	Serialized serialize();
	static NeuronConnection deserialize(Serialized &data);
};

class NeuralNetwork;

class Neuron
{
private:
	NeuronType _type;
	NeuralNetwork * _network;
	std::vector<NeuronConnection> _outputs;
	unsigned long int _id;
public:
	Neuron(NeuronType neuronType, NeuralNetwork *network, unsigned long int id = 0);
	const NeuronType &type = _type;
	const NeuralNetwork * network = _network;
	const std::vector<NeuronConnection> &outputs = _outputs;
	const unsigned long int &id = _id;
	NeuronConnection connectTo(Neuron *neuron);
	void addConnection(NeuronConnection connection);
	float value = 0;
	void update();

	struct Serialized {
		unsigned long int id;
		unsigned short int type;
		unsigned int numConnections;
		NeuronConnection::Serialized connections[];
	};

	Serialized serialize();
	static NeuronConnection deserialize(Serialized &data);
};

class NeuralNetwork
{

public:
	NeuralNetwork(ActivationFunction activationFunction = reluDefaultActivation);
	~NeuralNetwork();
	ActivationFunction activationFunction;
	std::vector<Neuron *> neurons;
	std::vector<Neuron *> neuronsOfType(NeuronType type);
	void update();
	std::vector<float> processInput(std::vector<float> inputValues);

	struct Serialized {

	};

	Serialized serialize();
	static NeuronConnection deserialize(Serialized &data);
};

#endif