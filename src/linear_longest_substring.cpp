#include "linear_longest_substring.hpp"
#include <vector>
#include <algorithm>

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

bool intersect(CommonSubArr a, CommonSubArr b){
    bool strA = a.startA <= b.startA + b.length && b.startA <= a.startA + a.length;
    bool strB = a.startB <= b.startB + b.length && b.startB <= a.startB + a.length;;
    return strA || strB;
}

bool compareByLength(const CommonSubArr &a, const CommonSubArr &b){
    return a.length < b.length;
}

vector<CommonSubArr> longest_common_substring(int *suffixArr, int* lcpArr, int size, int sizeA, int threshold){
    vector<CommonSubArr> substrings;
    
    for(int i=1; i<size; i++){
        if(lcpArr[i]>threshold){
            int a = suffixArr[i];
            int b = suffixArr[i-1];
            CommonSubArr cur = (struct CommonSubArr){0, 0, 0};
            if((a<sizeA && b>sizeA)){
                cur.startA = a; cur.startB = b;
                cur.length = lcpArr[i];
            }
            else if((b<sizeA && a>sizeA)){
                cur.startA = b; cur.startB = a;
                cur.length = lcpArr[i];
            }
            if(cur.startA + cur.startB + cur.length > 0){
                bool assigned = false;
                for(int j=0; j<substrings.size(); j++){
                    if(intersect(cur, substrings[j])){
                        if(substrings[j].length < cur.length)
                            substrings[j] = cur;
                        assigned = true;
                        break;
                    }
                }
                if(!assigned){
                    substrings.push_back(cur);
                }
            }

        }
    }
    sort(substrings.begin(), substrings.end(), compareByLength);
    return substrings;
}


