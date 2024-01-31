#include <cstring>
#include <stdexcept>
#include "utils.hpp"
#include "NeuralNetwork.hpp"

Neuron::Neuron(NeuronType neuronType, NeuralNetwork *network, size_t id)
	: _id(id), network(network), type(neuronType)
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

void Neuron::update(unsigned max_depth)
{
	if (network == nullptr)
	{
		throw new std::runtime_error("Invalid network");
	}
	if (--max_depth == 0)
	{
		return;
	}
	float activation = network->activationFunction()(value);

	for (Neuron::Connection &output : outputs)
	{
		float plasticityEffect = (std::abs(output.strength) > output.plasticityThreshold) ? output.plasticityRate : 1;
		float outputEffect = activation * output.strength * plasticityEffect * output.reliability;
		Neuron &neuron = network->get(output.neuron);
		neuron.value += outputEffect;
		neuron.update(max_depth);
	}
}

void Neuron::mutate()
{
	if (network == nullptr)
	{
		throw new std::runtime_error("Invalid network");
	}
	unsigned _rand = rand_seeded<unsigned>();
	size_t targetID = _rand % network->size();
	Neuron &neuron = network->get(targetID);
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
