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

NeuronConnection NeuronConnection::deserialize(NeuronConnection::Serialized &data)
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

/*json NeuronConnection::to_json()
{
	return json{
		{"neuron", neuron->id},
		{"strength", strength},
		{"plasticityRate", plasticityRate},
		{"plasticityThreshold", plasticityThreshold},
		{"reliability", reliability},
	};
}

NeuronConnection NeuronConnection::from_json(json &data)
{
	NeuronConnection conn;
	data["strength"].get_to(conn.strength);
	data["plasticityRate"].get_to(conn.plasticityRate);
	data["plasticityThreshold"].get_to(conn.plasticityThreshold);
	data["reliability"].get_to(conn.reliability);
	return conn;
}*/

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

/*json Neuron::to_json()
{
	json data = {
		{"id", id},
		{"type", type}};

	if (type != NeuronType::OUTPUT)
	{
		std::vector<json> outputsJson;

		for (NeuronConnection &output : _outputs)
		{
			outputsJson.push_back(output.to_json());
		}
		data["outputs"] = outputsJson;
	}

	return data;
}

Neuron Neuron::from_json(json &data, NeuralNetwork *network)
{
	Neuron *neuron = new Neuron(data["type"].get<NeuronType>(), network, data["id"].get<unsigned long int>());
	data.at("value").get_to(neuron->value);
	for (json &output : data["outputs"].get<std::vector<json>>())
	{
		NeuronConnection conn = NeuronConnection::from_json(output);
		neuron->addConnection(conn);
	}
	return *neuron;
}*/

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

/*json NeuralNetwork::to_json()
{
	std::vector<json> outputsJson;

	for (Neuron *const &neuron : neurons)
	{
		outputsJson.push_back(neuron->to_json());
	}

	return json{
		{"neurons", outputsJson}};
}

NeuralNetwork NeuralNetwork::from_json(json &j)
{
	NeuralNetwork network;
	auto outputsJson = j["neurons"].get<std::vector<json>>();
	for (json &n : outputsJson)
	{
		Neuron neuron = Neuron::from_json(n, &network);
		network.neurons.push_back(&neuron);
	}

	return network;
}*/