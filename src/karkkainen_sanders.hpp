
#ifndef DEFINED_KARKKAINEN_SANDERS_HPP
#define DEFINED_KARKKAINEN_SANDERS_HPP


bool lt(int a1, int b1, int a2, int b2, int a3 = 0, int b3 = 0);
bool lt_arr(int *a, int *b);
int fix(int p);
int* sort_triples(int *s, int *p12, int n12, int sigma);
int* sort_group0(int *s, int*p0, int n0, int sigma);
int assign_names(int *s, int *names, int *p12, int n12);
int* karkkainen_sanders_sa(int *s, int n, int sigma_size);

#endif
