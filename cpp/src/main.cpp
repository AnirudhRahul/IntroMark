#include <chromaprint.h>
#include <audio/Audio.hpp>
#include <../libs/large-alphabet-suffix-array/src/karkkainen_sanders.hpp>
#include <linear_longest_substring.hpp>
#include <iostream>
#include<fstream>
#include <math.h>
#include <algorithm>
#include <map>
#include <vector>
#include <cstring>


using std::cout; using std::endl; using std::sort; using std::tuple; using std::vector;
namespace fs = std::filesystem;

// Assert macro from https://stackoverflow.com/questions/3767869/adding-message-to-assert
#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

tuple<int*, uint32_t*, int> compress(uint32_t* arr, int size){
    uint32_t* sorted = new uint32_t[size];
    std::copy_n(arr, size, sorted);
    sort(sorted, sorted+size);
    std::map<uint32_t, int> rankMap;
    // Rank 0 is reserved for sentinels
    int rank = 2;
    for(uint32_t i = 0; i < size; i++){
        if(i==0 || sorted[i]!=sorted[i-1]){
            rankMap[sorted[i]] = rank;
            rank++;
        }
    }
    delete[] sorted;
    int* res = new int[size+3];
    res[size] = res[size+1] = res[size+2] = 0;
    for(int i=0; i < size; i++){
        res[i] = rankMap[arr[i]];
    }
    delete[] arr;

    uint32_t* rank_to_val = new uint32_t[rank];
    for (auto const& x : rankMap){
        rank_to_val[x.second] = x.first;
    }

    rankMap.clear();
    return std::make_tuple(res, rank_to_val, rank);
}

double compare_gray_codes(uint32_t a, uint32_t b){
    uint32_t c = a ^ b;
    double matches = 0;
    for(int i=0; i<16; i++){
        if( (c&3) == 0)
            matches+=1;
        c>>=2;
    }

    return matches/16;
}


// Only reads 2 files at a time to lower memory usage
vector<vector<TimeRange>> findSubstrings(vector<char*> pathList, bool verbose = false){
    vector<vector<TimeRange>> result;
    int renewIndex = -1;
    Audio audioList[2] = {Audio(pathList[0]), Audio(pathList[1])};

    int delay=-1; int item_duration=-1;
    
    //Initiliaze first file in the list
//    audioList[0] = audioFileToArr(pathList[0]);
    int sample_rate = audioList[0].sample_rate;
    int channels = 1;
    

//    for(int pathIndex=1; pathIndex < pathList.size(); pathIndex++){
//        if(renewIndex<0){
//            audioList[0] = audioFileToArr(pathList[pathIndex-1]);
//            audioList[1] = audioFileToArr(pathList[pathIndex]);
//            sample_rate = audioList[0].sample_rate;
//        }
//        else{
//            audioList[renewIndex] = audioFileToArr(pathList[pathIndex]);
//        }
//        for(int k=0;k<2;k++){
//            ASSERT(sample_rate == audioList[k].sample_rate, "Tracks must have the same sample rate");
//        }
//        cout << channels << " " << sample_rate << endl;
//        cout << audioList[0].filename << " " << audioList[1].filename << endl;
//
//
//        fs::path p = pathList[pathIndex];
//        cout << p.replace_extension("chroma1") << endl;
//
//
//
//
//        cout << "\nDone\n";
//
//        int combinedLen = chroma[0].size + chroma[1].size + 1;
//        int offset = chroma[0].size + 1;
//        uint32_t * merged = new uint32_t[combinedLen];
//        std::copy(chroma[0].arr, chroma[0].arr + chroma[0].size, merged);
//        std::copy(chroma[1].arr, chroma[1].arr + chroma[1].size, merged + offset);
//        freeChromaArr(&chroma[(renewIndex+1)%2]);
//
//        int max; int* compressed; uint32_t* rank_to_val;
//        std::tie(compressed, rank_to_val, max) = compress(merged, combinedLen);
//
//        // Sentinel in between the 2 strings
//        compressed[chroma[0].size] = 0;
//        if(verbose) cout << "Finished compressing\n";
//        int* suffixArr = karkkainen_sanders_sa(compressed, combinedLen, max);
//        if(verbose) cout << "Made suffix array of length " << combinedLen << endl;
//        int* rankArr = create_rank_arr(suffixArr, combinedLen);
//        // lcp in range from [1, combinedLen)
//        int* lcpArr =  create_lcp_arr(suffixArr, rankArr, compressed, combinedLen);
//        delete[] rankArr;
//        if(verbose) cout << "Made LCP array\n";
//
//        auto compareIndices = [compressed, rank_to_val](int a, int b) {
//            return compare_gray_codes(rank_to_val[compressed[a]], rank_to_val[compressed[b]]);
//        };
//        auto toSec = [item_duration, sample_rate](int in) {
//            return (double) in * item_duration / sample_rate;
//        };
//
//        int threshold = 0;
//        cout << "THRESH" << threshold << endl;
//        vector<CommonSubArr> common_substring_list = longest_common_substring(suffixArr, lcpArr, combinedLen, chroma[0].size, threshold);
//        delete[] suffixArr;
//        delete[] lcpArr;
//        cout << "OLD LEN " << common_substring_list.size() << endl;
//
//        int delay_item = delay/item_duration;
//        int mergeThreshold = 5*delay_item;
//        int offsetThreshold = std::max(2, (int)(0.25 * sample_rate / item_duration));
//
//        if(common_substring_list.size()>0)
//        for(int i=0;i<125;i++){
//            common_substring_list.push_back((struct CommonSubArr){
//                common_substring_list.back().startA + delay_item/30,
//                common_substring_list.back().startB + delay_item/30,
//                0
//            });
//        }
//
//
//
//        cout << "OLD NEW LEN " << common_substring_list.size() << endl;
//
//        for(int i=common_substring_list.size()-1;i>0;i--){
//            CommonSubArr next = common_substring_list[i];
//            for(int k=i-1; k>=0; k--){
//                CommonSubArr cur = common_substring_list[k];
//                int gap = next.startA - cur.startA - cur.length;
//                if(gap<=mergeThreshold && abs(cur.startA - cur.startB - (next.startA - next.startB)) <= offsetThreshold){
//                    if(gap>0){
//                        double match_measure = 0;
//                        for(int j=1; j<=gap; j++){
//                            match_measure+=compareIndices(next.startA-j, next.startB-j);
//                        }
//                        // cout << match_measure/gap << " " << cur.startA << " " << next.startA << endl;
//                        if(match_measure/gap < 0.75){
//                            continue;
//                        }
//                    }
//                    common_substring_list[i] = (struct CommonSubArr){
//                        cur.startA,
//                        cur.startB,
//                        std::min(next.startA + next.length - cur.startA, next.startB + next.length - cur.startB)
//                    };
//                    common_substring_list.erase(common_substring_list.begin()+k);
//                    break;
//                }
//                if(next.startA-cur.startA>=mergeThreshold)
//                    break;
//            }
//        }
//        delete[] compressed; delete[] rank_to_val;
//        cout << "MID LEN  " << common_substring_list.size() << endl;
//        common_substring_list.erase(
//        std::remove_if(common_substring_list.begin(), common_substring_list.end(),
//            [delay_item](const CommonSubArr & o) { return o.length <= delay_item; }),
//        common_substring_list.end());
//
//        cout << "NEW LEN " << common_substring_list.size() << endl;
//
//        // Add delay
//        for(CommonSubArr common: common_substring_list){
//            common.length += delay_item;
//            int endA = common.startA + common.length;
//            int endB = common.startB + common.length;
//            if(endA >= chroma[0].size || endB >= combinedLen){
//                continue;
//            }
//        }
//        double delay_sec = (double)delay/sample_rate;
//        vector<TimeRange> outputRanges[2];
//
//        for(CommonSubArr common: common_substring_list){
//            TimeRange curA = (struct TimeRange){
//                toSec(common.startA),
//                toSec(common.startA + common.length) + delay_sec * 1
//            };
//            outputRanges[0].push_back(curA);
//
//            TimeRange curB = (struct TimeRange){
//                toSec(common.startB - offset),
//                toSec(common.startB - offset + common.length) + delay_sec * 1
//            };
//            outputRanges[1].push_back(curB);
//        }
//
//        double secondMergeThreshold = delay_sec;
//        cout << "SEC " << secondMergeThreshold << endl;
//        for(int i=0;i<2;i++){
//            outputRanges[i].push_back((struct TimeRange){audioList[i].lengthSec, audioList[i].lengthSec});
//            sort(outputRanges[i].begin(), outputRanges[i].end(), sortByStart);
//            for(int k=outputRanges[i].size()-1; k>0; k--){
//                TimeRange cur = outputRanges[i][k-1]; TimeRange next = outputRanges[i][k];
//                if(next.start - cur.end <= secondMergeThreshold){
//                    outputRanges[i][k-1].end = next.end;
//                    outputRanges[i].erase(outputRanges[i].begin()+k);
//                }
//
//            }
//        }
//
//        // if(renewIndex<0){
//        //     co_yield outputRanges[0];
//        //     co_yield outputRanges[1];
//        // }
//        // else{
//        //     co_yield outputRanges[renewIndex];
//        // }
//        if(renewIndex<0){
//            result.push_back(outputRanges[0]);
//            result.push_back(outputRanges[1]);
//        }
//        else{
//            result.push_back(outputRanges[renewIndex]);
//        }
//        renewIndex = (renewIndex+1)%2;
//    }
//
//    for(int i=0;i<2;i++){
//        freeAudio(&audioList[i]);
//        freeChromaArr(&chroma[i]);
//    }


    return result;
}

/*
Command line options
    -f output to file
    -v verbose logs

The rest of the arguements should be a list of files in the order you want them compared.
*/
namespace fs = std::filesystem;
int main(int argc, char* argv[])
{
    vector<char*> pathList;
    char* outputFile;
    bool fileOutput = false;
    bool verbose = false;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"-f") || !strcmp(argv[i],"--file")){
            if(i+1>=argc){
                cout << "Must specify a filename when using -f\n";
                return EXIT_FAILURE;
            }
            i++;
            fileOutput = true;
            outputFile = argv[i];
        }
        else if(!strcmp(argv[i],"-v") || !strcmp(argv[i],"--verbose")){
            verbose = true;
        }
        else{
            pathList.push_back(argv[i]);
        }
    }

    if(pathList.size()<2){
        cout << "Not enough paths specified\n";
        return EXIT_FAILURE;
    }
    
    for(auto path : pathList){
        cout << fs::path(path).replace_extension("chroma1").string() << endl;
    }

    int index = 0;
    for(vector<TimeRange> common : findSubstrings(pathList, verbose)){
        cout << pathList[index] << endl;
        for(TimeRange cur:common)
            cout << cur.start << " to " << cur.end << endl;
        index++;
    }

    return 0;
}
