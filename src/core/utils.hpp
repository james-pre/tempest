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
#include <concepts>

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

bool debug = 0;
std::ostream &debug_output_default = std::cout;

template <typename... TData>
void log_debug(std::ostream &out, TData &&...data)
{
	if (!debug)
	{
		return;
	}

	out << "[debug] ";
	((out << std::forward<TData>(data)), ...);
	out << std::endl;
}

template <typename... TData>
void log_debug(TData &&...data)
{
	log_debug(debug_output_default, data...);
}

void log_debug(const std::string &message, std::ostream &out = debug_output_default)
{
	if (!debug)
	{
		return;
	}

	out << "[debug] " << message << std::endl;
}

#endif