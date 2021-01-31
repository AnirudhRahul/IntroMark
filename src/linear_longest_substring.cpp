#include "linear_longest_substring.hpp"
#include <tuple>

using namespace std;

int* create_rank_arr(int *suffixArr, int size){
    int* rank = new int[size];
    for(int i=0; i<size; i++){
        rank[suffixArr[i]] = i;
    }
    return rank;
}

int* create_lcp_arr(int *suffixArr, int* rankArr, int* arr, int size){
    int* lcp = new int[size]; lcp[0] = 0;
    int count=0;
    for(int i=0; i < size; i++){
        if(rankArr[i] > 0){
            int j = suffixArr[rankArr[i]-1];
            while(arr[i+count] == arr[j+count])
                count++; 
            lcp[rankArr[i]] = count;
            if(count>0) 
               count--;
        }
    }

    return lcp;
}

tuple<int, int, int, int> longest_common_substring(int *suffixArr, int* lcpArr, int size, int sizeA){
    int max = 0;
    int maxIndex = 0;
    int startA, startB, len=0;
    for(int i=1; i<size; i++){
        if(lcpArr[i]>len){
            int a = suffixArr[i];
            int b = suffixArr[i-1];
            if((a<sizeA && b>sizeA)){
                startA = a; startB = b;
                maxIndex = i;
                len = lcpArr[i];
            }
            else if((b<sizeA && a>sizeA)){
                startA = b; startB = a;
                maxIndex = i;
                len = lcpArr[i];
            }
        }
        if(lcpArr[i]>max)
            max = lcpArr[i];
    }

    return make_tuple(startA, startB, len, max);
}


