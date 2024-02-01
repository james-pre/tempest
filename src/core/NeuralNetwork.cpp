#include <cstring>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <algorithm>
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

void Neuron::update(unsigned depth)
{
	if (network == nullptr)
	{
		throw new std::runtime_error("Invalid network");
	}
	if (depth == 0 || depth > network->_max_depth)
	{
		return;
	}
	if (type == NeuronType::OUTPUT)
	{
		network->_notify();
		return;
	}

	float activation = network->activationFunction()(value);
	float outputEffect;

	for (Neuron::Connection &output : outputs)
	{
		Neuron &neuron = network->get(output.neuron);
		if (neuron.type == NeuronType::INPUT)
		{
			return;
		}
		// float plasticityEffect = (std::abs(output.strength) > output.plasticityThreshold) ? output.plasticityRate : 1;
		outputEffect = activation * output.strength * output.reliability;

		neuron.value *= outputEffect;
		neuron.update(--depth);
	}
}

void Neuron::mutate(BaseElement::MutationOptions options)
{
	if (network == nullptr)
	{
		throw new std::runtime_error("Invalid network");
	}
	size_t baseID = id(),
		   net_size = network->size(),
		   max = static_cast<size_t>(options.clumping / 2 * net_size);
	signed int adjustment = std::floor((0.5 - (baseID / (net_size - 1))) * options.clumping);
	size_t targetID = baseID + adjustment + (rand_seeded<float>() > 0.5f ? 1 : -1) * (rand_seeded<unsigned>() % max);
	targetID = std::clamp(targetID, 0ul, net_size - 1);
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
