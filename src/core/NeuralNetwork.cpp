#include <cstdlib>
#include <stdexcept>
#include "NeuralNetwork.hpp"

NeuronConnection::Serialized NeuronConnection::serialize()
{
	return NeuronConnection::Serialized
	{
		neuron->id,
		strength,
		plasticityRate,
		plasticityThreshold,
		reliability,
	};
}

NeuronConnection NeuronConnection::Deserialize(NeuronConnection::Serialized &data)
{
	return NeuronConnection
	{
		nullptr,
		data.strength,
		data.plasticityRate,
		data.plasticityThreshold,
		data.reliability,
	};
}

Neuron::Neuron(NeuronType neuronType, NeuralNetwork *network, unsigned long int id) : _type(neuronType), _network(network)
{
	_id = id ? id : reinterpret_cast<std::uintptr_t>(&*this);
}

NeuronConnection Neuron::connectTo(Neuron *neuron)
{
	NeuronConnection connection = {neuron};
	addConnection(connection);
	return connection;
}

void Neuron::addConnection(NeuronConnection connection)
{
	_outputs.push_back(connection);
}

void Neuron::update()
{
	float activation = network->activationFunction(value);

	for (NeuronConnection &output : _outputs)
	{
		float plasticityEffect = (std::abs(output.strength) > output.plasticityThreshold) ? output.plasticityRate : 1;
		float outputEffect = activation * output.strength * plasticityEffect * output.reliability;
		output.neuron->value += outputEffect;
		output.neuron->update();
	}
}

NeuralNetwork::NeuralNetwork(ActivationFunction activationFunction) : activationFunction(activationFunction) {}

NeuralNetwork::~NeuralNetwork()
{
	// Clean up allocated neurons
	for (Neuron *neuron : neurons)
	{
		delete neuron;
	}
}

std::vector<Neuron *> NeuralNetwork::neuronsOfType(NeuronType type)
{
	std::vector<Neuron *> ofType;
	for (Neuron *neuron : neurons)
	{
		if (neuron->type == type)
		{
			ofType.push_back(neuron);
		}
	}

	return ofType;
}

void NeuralNetwork::update()
{
	// Start the update process from input neurons
	for (Neuron *inputNeuron : neuronsOfType(NeuronType::INPUT))
	{
		inputNeuron->update();
	}
}

std::vector<float> NeuralNetwork::processInput(std::vector<float> inputValues)
{
	std::vector<Neuron *> inputs = neuronsOfType(NeuronType::INPUT);
	if (inputValues.size() != inputs.size())
	{
		throw new std::invalid_argument("Input size does not match the number of input neurons.");
	}

	// Set input values for input neurons
	for (size_t i = 0; i < inputValues.size(); ++i)
	{
		inputs[i]->value = inputValues[i];
	}

	// Update the network
	update();

	// Collect output values from output neurons
	std::vector<float> outputValues;
	for (Neuron *output : neuronsOfType(NeuronType::OUTPUT))
	{
		outputValues.push_back(output->value);
	}

	return outputValues;
}
