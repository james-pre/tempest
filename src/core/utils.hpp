#ifndef H_utils
#define H_utils

template <class element_t>
element_t nextTo(std::vector<element_t> &vector, element_t &element, typename std::iterator_traits<typename std::vector<element_t>::iterator>::difference_type offset = 1)
{
	return *std::next(std::find(vector.begin(), vector.end(), element), offset);
};

#endif