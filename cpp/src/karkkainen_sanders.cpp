#include <cstring>
#include <cmath>
#include "karkkainen_sanders.hpp"

using namespace std;

int int_size = sizeof(int);

int fix(int p) {
    return (2*p)/3;
}

bool lt(int a1, int b1, int a2, int b2, int a3, int b3) {
    return a1 != b1 ? a1 < b1 : (a2 != b2 ? a2 < b2 : a3 < b3);
}

bool lt_arr(int *a, int *b) {
    return a[0] != b[0] ? a[0] < b[0] : (a[1] != b[1] ? a[1] < b[1] : a[2] < b[2]);
}

void counting_sort(int *keys, int *a, int *b, int off, int n, int sigma) {
    int* count = new int[sigma + 1];
    memset(count, 0, int_size * (sigma + 1));

    for (int i = 0; i < n; ++i)
        count[keys[a[i] + off]]++;
    for (int d = 0, sum = 0; d <= sigma; ++d)
        sum += count[d], count[d] = sum - count[d];
    for (int i = 0; i < n; ++i)
        b[count[keys[a[i] + off]]++] = a[i];

    delete[] count;
}

int* sort_triples(int *s, int *p12, int n12, int sigma) {
    int* tmp = new int[n12];
    memset(tmp, 0, int_size * n12);

    counting_sort(s, p12, tmp, 2, n12, sigma);
    counting_sort(s, tmp, p12, 1, n12, sigma);
    counting_sort(s, p12, tmp, 0, n12, sigma);

    delete[] p12;
    // should be assigned to p12
    return tmp;
}

int* sort_group0(int *s, int *p0, int n0, int sigma) {
    int* tmp = new int[n0];
    memset(tmp, 0, int_size * n0);

    counting_sort(s, p0, tmp, 0, n0, sigma);
    delete[] p0;
    // should be assigned to p0
    return tmp;
}

int assign_names(int *s, int *names, int *p12, int n12) {
    int lex_name = 0, prev[3] = {-1, -1, -1};
    for (int i = 0; i < n12; ++i) {
        int *curr = &s[p12[i]];
        if (lt_arr(prev, curr))
            lex_name++;
        memcpy(prev, curr, int_size * 3);
        names[i] = lex_name;
    }
    return lex_name;
}

/*
Time Complexity: O(n + sigma)
Additional Space Complexity: O(3n + sigma)
s is the input array with values in the range [1, sigma]
n is the length of your input array exlcuding extra 0's for padding
sigma is the max value in s
*/
int* karkkainen_sanders_sa(int *s, int n, int sigma) {
    int n0 = (n + 2) / 3, n1 = n0, n2 = n / 3;
    int n12 = n1 + n2;

    // Get the indices of elements in group 0, and in groups 1 and 2
    int* p12 = new int[n12];
    for (int i = 1, j = 0; j < n12; i += i % 3, j++)
        p12[j] = i;

    // Sort triples of groups 1 and 2
    p12 = sort_triples(s, p12, n12, sigma);

    // Assign names to triples in groups 1 and 2
    int* names = new int[n12];
    int name_count = assign_names(s, names, p12, n12);

    // We now proceed to find the relative order of suffixes in groups 1 and 2
    int* sa12;

    // If names are not all distinct, find their sorted order recursively
    if (name_count < n12) {
        // Initialize string s12 of size 2n/3 from the lexicographic names
        int* s12 = new int[n12 + 3];
        memset(s12, 0, int_size * n12);
        s12[n12] = s12[n12 + 1] = s12[n12 + 2] = 0;
        for (int i = 0; i < n12; ++i)
            s12[p12[i] / 3 + (p12[i] % 3 == 1 ? 0 : n1)] = names[i];
        delete[] names;
        // Call the function recursively on s12
        sa12 = karkkainen_sanders_sa(s12, n12, name_count);
        delete[] s12;

        // Get sorted order of groups 1 and 2 from the suffix array of s12
        for (int i = 0; i < n12; ++i)
            p12[i] = (sa12[i] < n1 ? 1 + 3 * sa12[i] : 2 + 3 * (sa12[i] - n1));
    }
    else{
      delete[] names;
      sa12 = new int[n12];
      memset(sa12, 0, int_size * n12);
    }

    // Assign sa12 from the sorted order of suffixes
    for (int i = 0; i < n12; ++i)
        sa12[fix(p12[i])] = i;

    // Next step is sorting the suffixes in group 0
    int* p0 = new int[n0];
    // memset(p0, 0, int_size * n0);

    // Order in S(i+1) is implicit from sa12
    for (int i = 0, j = 0; j < n0; ++i)
      if (p12[i] % 3 == 1)
        p0[j++] = p12[i] - 1;

    // Sort suffixes in group 0
    p0 = sort_group0(s, p0, n0, sigma);

    // We now proceed for the merge step
    int i = 0, i0 = 0, i12 = (n % 3 == 1 ? 1 : 0);
    int a1=-1, a2=-1, a3=-1, b1=-1, b2=-1, b3=-1;
    int* sa = new int[n+3];
    sa[n] = sa[n+1] = sa[n+2] = -1;
    // Compare suffixes in group 0 against groups 1 and 2
    while (i0 < n0 && i12 < n12) {
        // Comparison to suffix in group 1
        if (p12[i12] % 3 == 1) {
            a1 = s[ p0[ i0]], a2 = sa12[fix( p0[ i0] + 1)];
            b1 = s[p12[i12]], b2 = sa12[fix(p12[i12] + 1)];
        }
        // Comparison to suffix in group 2
        else {
            a1 = s[ p0[ i0]], a2 = s[ p0[ i0] + 1], a3 = sa12[fix( p0[ i0] + 2)];
            b1 = s[p12[i12]], b2 = s[p12[i12] + 1], b3 = sa12[fix(p12[i12] + 2)];
        }
        sa[i++] = lt(a1, b1, a2, b2, a3, b3) ? p0[i0++] : p12[i12++];
    }
    delete[] sa12;

    // Copy the remaining suffixes
    // for (;  i0 <  n0;  ++i0) sa[i++] =  p0[ i0];
    memcpy(sa + i, p0 + i0, int_size*(n0-i0));
    delete[] p0;
    // for (; i12 < n12; ++i12) sa[i++] = p12[i12];
    memcpy(sa + i, p12 + i12, int_size*(n12-i12));
    delete[] p12;
    return sa;
}
