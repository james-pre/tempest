#include <ctime>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "NeuralNetwork.hpp"

NeuronConnection::Serialized NeuronConnection::serialize()
{
	return NeuronConnection::Serialized{
		neuron->id(),
		strength,
		plasticityRate,
		plasticityThreshold,
		reliability,
	};
}

NeuronConnection NeuronConnection::Deserialize(NeuronConnection::Serialized &data, Neuron &neuron)
{
	return NeuronConnection{
		&neuron,
		data.strength,
		data.plasticityRate,
		data.plasticityThreshold,
		data.reliability,
	};
}

// Todo: This should be changed to properly account for the different attributes
void NeuronConnection::mutate()
{
	std::srand(static_cast<unsigned>(std::time(0)));
	float mutationStrength = static_cast<float>(std::rand()) / 2;
	float newValue = static_cast<float>(std::rand()) - .5;
	switch (std::rand() % 5)
	{
	case 0:
		return;
	case 1:
		strength += newValue * mutationStrength;
		return;
	case 2:
		plasticityRate += newValue * mutationStrength;
		return;
	case 3:
		plasticityThreshold += newValue * mutationStrength;
		return;
	case 4:
		reliability += newValue * mutationStrength;
		return;
	}
}

Neuron::Neuron(NeuronType neuronType, NeuralNetwork &network, uint64_t id) : _type(neuronType), network(network)
{
	_id = id ? id : network.size();
}

size_t Neuron::id() const
{
	if(network)
	{
		throw std::runtime_error("Not attached to network");
	}

	return network.idOf(this);
}

NeuronConnection Neuron::connect(Neuron &neuron)
{
	NeuronConnection connection = {&neuron};
	addConnection(connection);
	return connection;
}

void Neuron::unconnect(Neuron &neuron)
{
	const auto it = std::find_if(_outputs.begin(), _outputs.end(), [&](NeuronConnection &conn)
								 { return conn.neuron->id() == neuron.id(); });

	if (it == _outputs.end())
	{
		throw std::runtime_error("Neuron is not connected");
	}

	removeConnection(static_cast<NeuronConnection>(*it));
}

void Neuron::addConnection(NeuronConnection connection)
{
	_outputs.push_back(connection);
}

void Neuron::removeConnection(const NeuronConnection &connection)
{
	std::remove(_outputs.begin(), _outputs.end(), connection);
}

void Neuron::update()
{
	float activation = network.activationFunction(value);

	for (NeuronConnection &output : _outputs)
	{
		float plasticityEffect = (std::abs(output.strength) > output.plasticityThreshold) ? output.plasticityRate : 1;
		float outputEffect = activation * output.strength * plasticityEffect * output.reliability;
		output.neuron->value += outputEffect;
		output.neuron->update();
	}
}

void Neuron::mutate()
{
	std::srand(static_cast<unsigned>(std::time(0)));
	int neuronCount = network.size();
	int id = std::rand() % neuronCount;
	Neuron &neuron = network.neuron(id);
	if (static_cast<float>(std::rand()) > 0.5)
	{
		connect(neuron);
	}
	else
	{
		unconnect(neuron);
	}
}

Neuron::Serialized Neuron::serialize()
{
	std::vector<NeuronConnection::Serialized> serializedOutputs;
	for(NeuronConnection &conn : _outputs)
	{
		serializedOutputs.push_back(conn.serialize());
	}
	
	return {
		_id,
		static_cast<uint16_t>(_type),
		_outputs.size(),
		serializedOutputs.data()
	};
}

Neuron Neuron::Deserialize(Neuron::Serialized &data, NeuralNetwork &network)
{
	Neuron neuron(static_cast<NeuronType>(data.type), network, data.id);
	for (size_t i = 0; i < data.num_outputs; ++i)
    {
        neuron.addConnection(NeuronConnection::Deserialize(data.outputs[i], network.neuron(data.outputs[i].neuron)));
    }
	return neuron;
}

NeuralNetwork::NeuralNetwork(ActivationFunction activationFunction, uint64_t id) : activationFunction(activationFunction) {
	_id = id ? id : reinterpret_cast<std::uintptr_t>(&*this);
}

NeuralNetwork::~NeuralNetwork()
{
}

Neuron &NeuralNetwork::neuron(size_t id)
{
	if(id >= _neurons.size())
	{
		throw std::out_of_range("Invalid neuron ID");
	}
    
	return _neurons[id];
}

size_t NeuralNetwork::idOf(const Neuron &neuron) const
{
    auto it = std::find_if(_neurons.begin(), _neurons.end(),
                           [neuron](const Neuron &n) { return neuron == &n; });

    if (it == _neurons.end())
    {
		throw std::runtime_error("Neuron not found in network");
	}

    return std::distance(_neurons.begin(), it);
}

void NeuralNetwork::removeNeuron(size_t id)
{
    if (id >= _neurons.size())
    {
		throw std::out_of_range("Invalid neuron ID");
	}
    
	_neurons.erase(_neurons.begin() + id);
}

std::vector<Neuron> NeuralNetwork::neuronsOfType(NeuronType type)
{
	std::vector<Neuron> ofType;
	for (Neuron &neuron : _neurons)
	{
		if (neuron.type() == type)
		{
			ofType.push_back(neuron);
		}
	}

	return ofType;
}

void NeuralNetwork::update()
{
	// Start the update process from input neurons
	for (Neuron &inputNeuron : neuronsOfType(NeuronType::INPUT))
	{
		inputNeuron.update();
	}
}

void NeuralNetwork::mutate()
{
}

std::vector<float> NeuralNetwork::processInput(std::vector<float> inputValues)
{
	std::vector<Neuron> inputs = neuronsOfType(NeuronType::INPUT);
	if (inputValues.size() != inputs.size())
	{
		throw new std::invalid_argument("Input size does not match the number of input neurons.");
	}

	// Set input values for input neurons
	for (size_t i = 0; i < inputValues.size(); ++i)
	{
		inputs[i].value = inputValues[i];
	}

	// Update the network
	update();

	// Collect output values from output neurons
	std::vector<float> outputValues;
	for (Neuron &output : neuronsOfType(NeuronType::OUTPUT))
	{
		outputValues.push_back(output.value);
	}

	return outputValues;
}

NeuralNetwork::Serialized NeuralNetwork::serialize()
{
	std::vector<Neuron::Serialized> serializedNeurons;

	for (Neuron &neuron : _neurons)
    {
        serializedNeurons.push_back(neuron.serialize());
    }
	
	return {
		_id,
		size(),
		serializedNeurons.data()
	};
}

NeuralNetwork NeuralNetwork::Deserialize(NeuralNetwork::Serialized &data)
{
	NeuralNetwork network(reluDefaultActivation, data.id);
	for(size_t i = 0; i < data.num_neurons; i++)
	{
		network.addNeuron(Neuron::Deserialize(data.neurons[i], network));
	}

	return network;
}