#include <cstring>
#include <stdexcept>
#include "utils.hpp"
#include "NeuralNetwork.hpp"

// Todo: This should be changed to properly account for the different attributes
void NeuronConnection::mutate()
{

	float mutationStrength = rand_seeded<float>() / 2;
	float newValue = rand_seeded<float>() - .5;
	switch (rand_seeded<int>() % 5)
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
	: _id(id != SIZE_MAX ? id : network.size()), network(&network), type(neuronType)
{
}

size_t Neuron::id() const
{
	if (network == nullptr)
	{
		throw new std::runtime_error("Invalid network");
	}
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
	float activation = network->activationFunction()(value);

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
	unsigned _rand = rand_seeded<unsigned>();
	size_t targetID = _rand % network->size();
	Neuron &neuron = network->neuron(targetID);
	if (type == NeuronType::OUTPUT || neuron.type == NeuronType::INPUT)
	{
		return;
	}

	if (rand_seeded<float>() > 0.5)
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

	float random = rand_seeded<float>();

	size_t target = rand_seeded<unsigned>() % size();
	if (random < .6)
	{
		neuron(target).mutate();
		return;
	}

	if (random < 0.75 && hasNeuron(target))
	{
		removeNeuron(target);
		return;
	}

	createNeuron(NeuronType::TRANSITIONAL);
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
