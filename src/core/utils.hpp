#ifndef H_utils
#define H_utils

#include <vector>
#include <ctime>
#include <cstdlib>
#include <string>

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

#define _MAPABLE_MEMBER(name, return_value) \
	if (key == #name)                       \
		return return_value;

#define _MAPABLE_MEMBERS_GET(name) _MAPABLE_MEMBER(name, name)
#define _MAPABLE_MEMBERS_SET(name) _MAPABLE_MEMBER(name, name = new_value)
#define _MAPABLE_MEMBERS_HAS(name) _MAPABLE_MEMBER(name, true)

#define MAPABLE_MEMBERS(args...)                                          \
	template <typename T>                                                 \
	T &get(const std::string &key)                                        \
	{                                                                     \
		FOREACH(_MAPABLE_MEMBERS_GET, args)                               \
		throw new std::runtime_error("member does not exist in mapable"); \
	}                                                                     \
	template <typename T>                                                 \
	void set(const std::string &key, T new_value)                         \
	{                                                                     \
		FOREACH(_MAPABLE_MEMBERS_SET, args)                               \
	}                                                                     \
	bool has(const std::string &key)                                      \
	{                                                                     \
		FOREACH(_MAPABLE_MEMBERS_HAS, args)                               \
		return false;                                                     \
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

#endif