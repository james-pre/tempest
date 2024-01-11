#ifndef H_utils
#define H_utils

#include <vector>

template <class element_t>
element_t nextTo(const std::vector<element_t> &vector, const element_t &element, const std::ptrdiff_t offset = 1)
{
	return *std::next(std::find(vector.begin(), vector.end(), element), offset);
};

#endif