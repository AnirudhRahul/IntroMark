
#ifndef DEFINED_LINEAR_SUBSTRING_HPP
#define DEFINED_LINEAR_SUBSTRING_HPP

#include <tuple>

// suffixArr must be a valid suffix array with values in the range [0, size-1]
int* create_rank_arr(int *suffixArr, int size);

// suffixArr must be a valid suffix array with values in the range [0, size-1]
// similiarly rankArr must be the inverse of suffixArr from the function above
int* create_lcp_arr(int *suffixArr, int* rankArr, int* arr, int size);

// startA, startB, len
std::tuple<int, int, int> longest_common_substring(int *suffixArr, int* lcpArr, int size, int sizeA);

#endif
