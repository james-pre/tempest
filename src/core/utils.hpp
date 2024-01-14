#ifndef H_utils
#define H_utils

#include <vector>
#include <ctime>
#include <cstdlib>



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

#endif