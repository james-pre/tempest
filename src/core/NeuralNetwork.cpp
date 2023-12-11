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
	_id = id ? id : reinterpret_cast<std::uintptr_t>(&*this);
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
	int neuronCount = network.neurons.size();
	int index = std::rand() % neuronCount;
	Neuron &neuron = network.neurons.at(index);
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
	std::vector<NeuronConnection::Serialized> serialziedOutputs;
	std::transform(_outputs.begin(), _outputs.end(), std::back_inserter(serialziedOutputs),
				   [](NeuronConnection &conn)
				   { return conn.serialize(); });
	
	Neuron::Serialized out = {
		_id,
		static_cast<uint16_t>(_type),
		_outputs.size(),
		new NeuronConnection::Serialized[_outputs.size()]
	};
	std::memcpy(out.outputs, serialziedOutputs.data(), sizeof(NeuronConnection::Serialized) * _outputs.size());
	return out;
}

Neuron Neuron::Deserialize(Neuron::Serialized &data, NeuralNetwork &network)
{
	return Neuron(static_cast<NeuronType>(data.type), network, data.id);
}

NeuralNetwork::NeuralNetwork(ActivationFunction activationFunction, uint64_t id) : activationFunction(activationFunction) {
	_id = id ? id : reinterpret_cast<std::uintptr_t>(&*this);
}

NeuralNetwork::~NeuralNetwork()
{
}

std::vector<Neuron> NeuralNetwork::neuronsOfType(NeuronType type)
{
	std::vector<Neuron> ofType;
	for (Neuron &neuron : neurons)
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
	std::vector<Neuron::Serialized> serialziedneurons;
	std::transform(neurons.begin(), neurons.end(), std::back_inserter(serialziedneurons),
				   [](Neuron &neuron)
				   { return neuron.serialize(); });
	
	NeuralNetwork::Serialized out = {
		_id,
		neurons.size(),
		serialziedneurons.data()
	};
	//std::memcpy(out.neurons, serialziedneurons.data(), sizeof(Neuron::Serialized) * neurons.size());
	return out;
}