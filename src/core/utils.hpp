#ifndef H_utils
#define H_utils

#include <vector>
#include <ctime>
#include <cstdlib>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include <sstream>
#include <iostream>

/*
Resolves enums
*/
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

#define _PARENS ()

// Rescan macro tokens 256 times
#define _FOREACH_EXPAND(...) _FOREACH_EXPAND1(_FOREACH_EXPAND1(_FOREACH_EXPAND1(_FOREACH_EXPAND1(__VA_ARGS__))))
#define _FOREACH_EXPAND1(...) _FOREACH_EXPAND2(_FOREACH_EXPAND2(_FOREACH_EXPAND2(_FOREACH_EXPAND2(__VA_ARGS__))))
#define _FOREACH_EXPAND2(...) _FOREACH_EXPAND3(_FOREACH_EXPAND3(_FOREACH_EXPAND3(_FOREACH_EXPAND3(__VA_ARGS__))))
#define _FOREACH_EXPAND3(...) _FOREACH_EXPAND4(_FOREACH_EXPAND4(_FOREACH_EXPAND4(_FOREACH_EXPAND4(__VA_ARGS__))))
#define _FOREACH_EXPAND4(...) __VA_ARGS__

/*
Foreach/recursive macro
See https://www.scs.stanford.edu/~dm/blog/va-opt.html
*/
#define FOREACH(macro, ...) \
	__VA_OPT__(_FOREACH_EXPAND(_FOREACH_HELPER(macro, __VA_ARGS__)))
#define _FOREACH_HELPER(macro, a, ...) \
	macro(a)                           \
		__VA_OPT__(_FOREACH _PARENS(macro, __VA_ARGS__))
#define _FOREACH() _FOREACH_HELPER

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
	T &get(const std::string &key)                                     \
	{                                                                  \
		FOREACH(_REFLECT_GET, args)                                    \
		throw new std::runtime_error("mapable member does not exist"); \
	}                                                                  \
	std::string get_string(const std::string &key)                     \
	{                                                                  \
		FOREACH(_REFLECT_GET_STRING, args)                             \
		throw new std::runtime_error("mapable member does not exist"); \
	}                                                                  \
	template <typename T>                                              \
	void set(const std::string &key, T new_value)                      \
	{                                                                  \
		FOREACH(_REFLECT_SET, args)                                    \
	}                                                                  \
	/*template <typename T>*/                                          \
	void set(const std::string &key, const std::string &new_value)     \
	{                                                                  \
		FOREACH(_REFLECT_SET, args)                                    \
	}                                                                  \
	bool has(const std::string &key)                                   \
	{                                                                  \
		FOREACH(_REFLECT_HAS, args)                                    \
		return false;                                                  \
	}

class Reflectable
{
public:
	template <typename T>
	T &get(const std::string &key)
	{
		throw new std::runtime_error("MapableMembers::get virtual call");
	}

	virtual std::string get_string([[maybe_unused]] const std::string &key)
	{
		throw new std::runtime_error("MapableMembers::get_string virtual call");
	}

	virtual void set([[maybe_unused]] const std::string &key, [[maybe_unused]] const std::string &new_value)
	{
		throw new std::runtime_error("MapableMembers::set virtual call");
	}

	virtual bool has([[maybe_unused]] const std::string &key)
	{
		throw new std::runtime_error("MapableMembers::has virtual call");
	}

	virtual ~Reflectable() = default;
};

template <typename T>
T from_string(T val)
{
	return val;
}

template <typename T>
typename std::enable_if<!std::is_enum<T>::value, T>::type
from_string(const std::string &str)
{
	T val;
	std::istringstream(str) >> val;
	return val;
}

template <typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type
from_string(const std::string &str)
{
	typename std::underlying_type<T>::type val;
	std::istringstream(str) >> val;
	return static_cast<T>(val);
}

template <class element_t>
element_t nextTo(const std::vector<element_t> &vector, const element_t &element, const std::ptrdiff_t offset = 1)
{
	return *std::next(std::find(vector.begin(), vector.end(), element), offset);
};

template <typename random_t>
random_t rand_seeded()
{
	[[maybe_unused]] static bool const _rand_seeded_is_seeded = (std::srand(static_cast<unsigned>(std::time(0))), true);
	return static_cast<random_t>(std::rand());
}

std::vector<std::string> split(const std::string &input, const std::string &delimiter, size_t start = 0)
{
	std::vector<std::string> tokens;
	size_t end = input.find(delimiter);

	while (end != std::string::npos)
	{
		tokens.push_back(input.substr(start, end - start));
		start = end + delimiter.length();
		end = input.find(delimiter, start);
	}

	tokens.push_back(input.substr(start, std::min(end, input.size()) - start));

	return tokens;
}

template <typename T>
T stonum(std::string &str)
{
	T value;
	std::stringstream(str) >> value;
	return value;
}

#endif