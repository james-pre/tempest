/*
Generic helper classes

Reflectable: for reflection
Mutatable: for things that have a mutate method
*/

#ifndef H_generic
#define H_generic

#include <string>
#include <stdexcept>
#include "utils.hpp"

#define _REFLECT_GET(name) \
	if (key == #name)      \
		return name;
#define _REFLECT_GET_STRING(name) \
	if (key == #name)             \
		return std::to_string(name);
#define _REFLECT_SET(name)                             \
	if (key == #name)                                  \
	{                                                  \
		name = from_string<decltype(name)>(new_value); \
	}
#define _REFLECT_HAS(name) \
	if (key == #name)      \
		return true;
#define _REFLECT_KEYS(name) #name,

#define REFLECT(args...)                                               \
	template <typename T>                                              \
	T &getProperty(const std::string &key)                                     \
	{                                                                  \
		FOREACH(_REFLECT_GET, args)                                    \
		throw new std::runtime_error("mapable member does not exist"); \
	}                                                                  \
	std::string getPropertyString(const std::string &key)                     \
	{                                                                  \
		FOREACH(_REFLECT_GET_STRING, args)                             \
		throw new std::runtime_error("mapable member does not exist"); \
	}                                                                  \
	template <typename T>                                              \
	void setProperty(const std::string &key, T new_value)                      \
	{                                                                  \
		FOREACH(_REFLECT_SET, args)                                    \
	}                                                                  \
	void setProperty(const std::string &key, const std::string &new_value)     \
	{                                                                  \
		FOREACH(_REFLECT_SET, args)                                    \
	}                                                                  \
	bool hasProperty(const std::string &key)                                   \
	{                                                                  \
		FOREACH(_REFLECT_HAS, args)                                    \
		return false;                                                  \
	}

class Reflectable
{
public:
	template <typename T>
	T &getProperty(const std::string &key)
	{
		throw new std::runtime_error("Reflectable::getProperty virtual call");
	}

	virtual std::string getPropertyString([[maybe_unused]] const std::string &key)
	{
		throw new std::runtime_error("Reflectable::getPropertyString virtual call");
	}

	virtual void setProperty([[maybe_unused]] const std::string &key, [[maybe_unused]] const std::string &new_value)
	{
		throw new std::runtime_error("Reflectable::setProperty virtual call");
	}

	virtual bool hasProperty([[maybe_unused]] const std::string &key)
	{
		throw new std::runtime_error("Reflectable::hasProperty virtual call");
	}

	Reflectable()
	{
	}

	virtual ~Reflectable() = default;
};

class Mutatable
{
public:
	virtual void mutate()
	{
		throw new std::runtime_error("Mutatable::mutate virtual call");
	}
};

#endif