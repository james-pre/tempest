#include <ctime>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "NeuralNetwork.hpp"

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

Neuron::Neuron(NeuronType neuronType, NeuralNetwork &network, size_t id)
		: _type(neuronType), _id(id != SIZE_MAX ? id : network.size()), network(&network) {}

size_t Neuron::id() const
{
	return network->idOf(this);
}

Neuron Neuron::Deserialize(Neuron::Serialized &data, NeuralNetwork &network)
{
	Neuron neuron(static_cast<NeuronType>(data.type), network, data.id);
	for (NeuronConnection::Serialized &conn : data.outputs)
	{
		neuron.addConnection(NeuronConnection::Deserialize(conn));
	}
	return neuron;
}

void Neuron::update()
{
	float activation = network->activationFunction(value);

	for (NeuronConnection &output : outputs)
	{
		float plasticityEffect = (std::abs(output.strength) > output.plasticityThreshold) ? output.plasticityRate : 1;
		float outputEffect = activation * output.strength * plasticityEffect * output.reliability;
		Neuron &neuron = network->neuron(output.neuron);
		neuron.value += outputEffect;
		neuron.update();
	}
}

void Neuron::mutate()
{
	std::srand(static_cast<unsigned>(std::time(0)));
	unsigned _rand = static_cast<unsigned>(std::rand());
	size_t targetID = _rand % network->size();
	Neuron &neuron = network->neuron(targetID);
	if (static_cast<float>(std::rand()) > 0.5)
	{
		connect(neuron);
	}
	else
	{
		unconnect(neuron);
	}
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
	std::srand(static_cast<unsigned>(std::time(0)));
	
	float rand = ((float) std::rand() / (RAND_MAX));

	size_t target = std::rand() % size();
	if (rand < .6)
	{
		neuron(target).mutate();
		return;
	}
	
	if(rand < 0.75 && hasNeuron(target))
	{
		removeNeuron(target);
		return;
	}

	Neuron newNeuron(NeuronType::TRANSITIONAL, *this);
	addNeuron(newNeuron);
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
