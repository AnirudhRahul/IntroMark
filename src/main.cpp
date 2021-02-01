#include <chromaprint.h>
#include <AudioFile.h>
#include <karkkainen_sanders.hpp>
#include <linear_longest_substring.hpp>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <map>

using std::cout, std::endl, std::sort, std::tuple;

// Assert macro from https://stackoverflow.com/questions/3767869/adding-message-to-assert
#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

struct Audio{
    int16_t* arr;
    int sample_rate;
    int samples;
    int channels;
    int length;
};

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

Audio audioFileToArr(const char * path){
    AudioFile<float> audioFile;
    audioFile.load(path);
    
    int samples = audioFile.getNumSamplesPerChannel();
    int channels = audioFile.getNumChannels();

    int16_t* arr = new int16_t[samples*channels];

    int index = 0;
    for (int i = 0; i < samples; i++){
        for(int j = 0; j < channels; j++){
            arr[index] = round(audioFile.samples[j][i] * 32767); 
            index++;
        }
    }

    return (struct Audio){arr, (int)audioFile.getSampleRate(), samples, channels, samples*channels};
}

void freeAudio(Audio input){
    delete[] input.arr;
}

int main()
{
    ChromaprintContext *ctx;
    const char* path1 = "../test_audio/normal_ep1.wav";
    const char* path2 = "../test_audio/normal_ep2.wav";

    Audio audioList[] = {audioFileToArr(path1), audioFileToArr(path2)};
    cout << "Read audio\n";
    uint32_t* audioHashes[2];
    int hashSizes[2];
    int delay, item_duration, sample_rate;
    int index = 0;
    for(Audio cur : audioList){
        ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_TEST5);
        chromaprint_start(ctx, cur.sample_rate, cur.channels);
        chromaprint_feed(ctx, cur.arr, cur.length);
        freeAudio(cur);
        chromaprint_finish(ctx);
        if(index==0){
            sample_rate = cur.sample_rate;
            delay = chromaprint_get_delay(ctx);
            item_duration = chromaprint_get_item_duration(ctx);
            cout << "DELAY SEC " << (double) sample_rate / delay << endl;
        }
        else{
            ASSERT(sample_rate == cur.sample_rate, "Sample rates are different\n");
            ASSERT(delay == chromaprint_get_delay(ctx), "Delay Mismatch\n");
            ASSERT(item_duration == chromaprint_get_item_duration(ctx), "Item Duration Mismatch\n");
        }

        chromaprint_get_raw_fingerprint(ctx, &audioHashes[index], &hashSizes[index]);
        chromaprint_free(ctx);
        index++;
    }
    cout << "Chromaprint done!\n";

    int combinedLen = hashSizes[0] + hashSizes[1] + 1;
    uint32_t * merged = new uint32_t[combinedLen];
    std::copy(audioHashes[0], audioHashes[0] + hashSizes[0], merged);
    delete[] audioHashes[0];
    std::copy(audioHashes[1], audioHashes[1] + hashSizes[1], merged + hashSizes[0] + 1);
    delete[] audioHashes[1];

    int max; int* compressed; uint32_t* rank_to_val;
    std::tie(compressed, rank_to_val, max) = compress(merged, hashSizes[0]+hashSizes[1]+1);
    // Sentinel in between the 2 strings
    compressed[hashSizes[0]] = 0;

    cout << "Finished compressing\n";

    int* suffixArr = karkkainen_sanders_sa(compressed, combinedLen, max);

    cout << "Made suffix array\n";
    // for(int i=0; i<combinedLen ; i++){
    //     cout << suffixArr[i] << ", ";
    // }
    // cout << endl;

    int* rankArr = create_rank_arr(suffixArr, combinedLen);
    // lcp in range from [1, combinedLen)
    int* lcpArr =  create_lcp_arr(suffixArr, rankArr, compressed, combinedLen);
    cout << "Made LCP array\n";

    // for(int i=1; i<combinedLen ; i++){
    //     cout << lcpArr[i] << ", ";
    // }
    // cout << endl;

    int startA, startB, len, maxLCP;
    std::tie(startA, startB, len, maxLCP) = longest_common_substring(suffixArr, lcpArr, combinedLen, hashSizes[0]);
    cout << "Found least common substring\n";

    // cout << "SAMPLE RATE: " << sample_rate << endl;

    // converts item counts to secs
    auto toSec = [item_duration, sample_rate](int in) {
        return (double) in * item_duration / sample_rate;
    };

    auto compareIndices = [compressed, rank_to_val](int a, int b) {
        return compare_gray_codes(rank_to_val[compressed[a]], rank_to_val[compressed[b]]);
    };

    // cout << "ITEM DUR: " << toSec(1) << endl;
    // cout << "Delay SEC: " << (double) delay / sample_rate << endl;
    // cout << "Common SEC: " << toSec(len) << endl;
    // cout << toSec(hashSizes[0]) << " " << toSec(hashSizes[1]) << endl;

    double delay_sec = (double) delay / sample_rate;
    int sizeA = hashSizes[0];
    cout << "Max lcp in sec: " << toSec(maxLCP) << endl;
    cout << endl << "Common Substrings found" << endl;
    cout << "Length in sec: " << toSec(len) << endl;
    cout << toSec(startA) << " to " << toSec(startA + len) + delay_sec << endl;
    cout << toSec(startB - sizeA - 1) << " to " << toSec(startB + len - sizeA - 1) + delay_sec << endl;
    cout << endl;

    int endA = startA + len; int endB = startB + len;
    const int mismatch_threshold = 1;
    int mismatch_count=0;
    while(startA>=0 && startB>sizeA){
        if(compareIndices(startA, startB)<0.8){
            mismatch_count++;
        }
        // else{
        //     mismatch_count=std::max(0, mismatch_count-1);
        // }
        if(mismatch_count > mismatch_threshold){
            break;
        }
        // cout << compressed[startA] << " " << compressed[startB] << "\n";
        startA--;startB--;
    }
    while(compressed[startA]!=compressed[startB]){
        startA++; startB++;
    }
    mismatch_count = 0;
    endA-=10; endB-=10;

    while(endA<sizeA && endB<combinedLen){
        if(compareIndices(endA, endB)<0.8){
            mismatch_count++;
        }
        // else{
        //     mismatch_count=std::max(0, mismatch_count-1);
        // }
        if(mismatch_count > mismatch_threshold){
            break;
        }
        // cout << compressed[endA] << " " << compressed[endB] << " " << mismatch_count << "\n";
        endA++;endB++;
    }
    while(compressed[endA]!=compressed[endB]){
        endA--; endB--;
    }
    cout  << compressed[endA] << " comp " << compressed[endB] << endl;
    // cout << "END: " << toSec(sizeA) << endl;
    cout << toSec(startA) << " to " << toSec(endA) + delay_sec << endl;
    // cout << "END: " << toSec(combinedLen) << endl;
    cout << toSec(startB - sizeA - 1) << " to " << toSec(endB - sizeA - 1) + delay_sec << endl;
    cout << endl;

    cout << "NEW LEN: " << toSec(endA-startA) + delay_sec << endl;

    cout << "DELAY: " << delay_sec << endl;

    cout << "SINGLE SAMPLE LENGTH: " << toSec(1) << endl;
    for(int i=startA-10; i<endA; i++){
        cout << compressed[i] << " ";
    }
    cout << "\n\n";
    for(int i=startB-10; i<endB; i++){
        cout << compressed[i] << " ";
    }
    cout << "\n\n";

    // O(n) double check
    // int foundIndex = -1;
    // for(int i=sizeA; i<combinedLen; i++){
    //     bool worked = true;
    //     for(int j=0; j<len-1; j++){
    //         if(compressed[startA+j]!=compressed[i+j]){
    //             worked = false;
    //             break;
    //         }
    //     }
    //     if(worked){
    //         foundIndex = i;
    //         break;
    //     }
    // }
    // while(foundIndex>sizeA && startA>0 && compressed[foundIndex]==compressed[startA]){
    //     foundIndex--;startA--;
    // }
    // foundIndex+=1;startA+=1;

    // for(int i = startA; i < startA+10; i++)
    //     cout << compressed[i] << " ";
    // cout << endl;
    // cout << endl;

    // if(foundIndex!=-1)
    //     for(int i = foundIndex; i < foundIndex+10; i++)
    //         cout << compressed[i] << " ";
    // cout << endl;

    // for(int i = startA-20; i < startA+1; i++){
    //     cout << compressed[i] << " " << compressed[i+foundIndex-startA] << endl;
    // }
    
    // ASSERT(foundIndex-sizeA-1==startB, "Substring matching error");

    // cout << toSec(startA) << " to " << toSec(startA + len) << endl;
    // cout << toSec(foundIndex-sizeA-1) << " to " << toSec(foundIndex-sizeA-1+len) << endl;

    return 0;
}
